#include <numeric>
#include <unordered_map>
#include <chrono>

#include "can/can_codec.h"
#include "v2c_transcoder.h"

namespace can {

template <typename T>
class sig_last {
	const tr_signal _sig;
	sig_calc_type<T> _val { 0 };
public:
	sig_last(const tr_signal sig) : _sig(sig) {}

	friend uint64_t tag_invoke(assemble_cpo, sig_last& self, uint64_t mux_val, uint64_t fd) {
		if (!self._sig.is_active(mux_val))
			return 0;

		sig_calc_type<T> val(self._sig.decode(fd));
		self._val = val;
		return self._sig.encode(val.get_raw());
	}

	friend void tag_invoke(reset_cpo, sig_last&) { }
};

template <typename T>
class sig_avg {
	const tr_signal _sig;
	sig_calc_type<T> _val { 0 };
	uint64_t _num_samples { 0 };
public:
	sig_avg(const tr_signal sig) : _sig(sig) {}

	friend uint64_t tag_invoke(assemble_cpo, sig_avg& self, uint64_t mux_val, uint64_t fd) {
		if (!self._sig.is_active(mux_val))
			return 0;

		sig_calc_type<T> val(self._sig.decode(fd));
		self._val = (self._num_samples == 0) ? val : self._val + val;
		++self._num_samples;

		return self._sig.encode(self._val.idivround(self._num_samples).get_raw());
	}

	friend void tag_invoke(reset_cpo, sig_avg& self) {
		self._val = 0;
		self._num_samples = 0;
	}
};

static int32_t millis_diff(can_time tp, uint32_t utc) {
	using namespace std::chrono;
	return duration_cast<milliseconds>(tp - can_time(seconds(utc))).count();
}

void tx_group::try_publish(can_time up_to, frame_packet& fp) {
	if (_group_origin + _assemble_freq <= up_to) {
		if (all_collected())
			publish(up_to, fp);
		_group_origin = up_to;
	}
}

void tx_group::publish(can_time tp, frame_packet& fp) {
	// for messages with muxed signals, non-muxed signal values should be taken 
	// from the frame with the latest timestamp, indicated by can::use_non_muxed(cf, true)

	std::sort(_msg_clumps.begin(), _msg_clumps.end(), [](const auto& a, const auto& b) {
		if (a.message_id != b.message_id) // primarily by message_ids
			return a.message_id < b.message_id;
		return a.stamp > b.stamp; // secondarily by timestamp, descending
		});

	int64_t prev_id = 0;
	for (const auto& smsg : _msg_clumps) {
		can_frame cf { 0 };
		cf.can_id = smsg.message_id;
		cf.len = CAN_MAX_DLEN;
		can::use_non_muxed(cf, prev_id != cf.can_id);
		prev_id = cf.can_id;
		auto raw_data = smsg.mdata;
		std::memcpy(cf.data, &raw_data, CAN_MAX_DLEN);
		fp.append(millis_diff(tp, fp.utc()));
		fp.append(cf);
	}
}

bool tx_group::all_collected() const {
	using namespace std::chrono;
	for (const auto& smsg : _msg_clumps) {
		auto d = duration_cast<milliseconds>(smsg.stamp - _group_origin);
		if (d < milliseconds(0) || d >= _assemble_freq)
			return false;
	}
	return true;
}

// used only to assemble messages

void tx_group::assign(canid_t message_id, int64_t message_mux) {
	_msg_clumps.push_back({ .message_id = message_id, .message_mux = message_mux });
}

bool tx_group::within_interval(can_time stamp) const {
	return stamp >= _group_origin && stamp < _group_origin + _assemble_freq;
}

void tx_group::add_clumped(can_time stamp, canid_t message_id, int64_t message_mux, uint64_t cval) {
	for (auto& smsg : _msg_clumps) {
		if (smsg.message_id == message_id && smsg.message_mux == message_mux) {
			smsg.stamp = stamp; smsg.mdata = cval;
			break;
		}
	}
}

template <template <typename> typename sig_agg>
static sig_asm make_sig_agg(const tr_signal& sig) {
	switch (sig.value_type()) {
		case i64: return sig_asm(sig_agg<int64_t>(sig));
		case u64: return sig_asm(sig_agg<uint64_t>(sig));
		case f32: return sig_asm(sig_agg<float>(sig));
		case f64: return sig_asm(sig_agg<double>(sig));
	}
	fprintf(stderr, "Signal %s doesn't have a valid value_type\n", sig.name().c_str());
	return { sig_avg<int64_t>(sig) };
}

std::vector<uint64_t> tr_message::distinct_mux_vals() const {
	std::vector<uint64_t> mux_vals;

	for (const auto& sig : _signals) {
		if (sig.mux_val().has_value())
			mux_vals.push_back(sig.mux_val().value());
	}

	std::sort(mux_vals.begin(), mux_vals.end());
	mux_vals.erase(std::unique(mux_vals.begin(), mux_vals.end()), mux_vals.end());

	return mux_vals;
}

void tr_message::assign_group(tx_group* txg, uint32_t message_id) {
	_tx_group = txg;
	make_sig_assemblers();

	if (_mux.has_value())
		for (const auto mux_val : distinct_mux_vals())
			_tx_group->assign(message_id, mux_val);
	else
		_tx_group->assign(message_id, -1);
}

void tr_message::assemble(can_time stamp, can_frame frame) {
	if (!_tx_group) return;

	if (!_tx_group->within_interval(_last_stamp))
		reset_sig_asms();

	uint64_t clumped_val = 0;
	uint64_t fd = *(const uint64_t*)frame.data;
	int64_t mux_val = _mux.has_value() ? _mux->decode(fd) : -1;

	for (auto& sc : _sig_asms)
		clumped_val |= sig_assemble(sc, mux_val, fd);

	if (_mux.has_value())
		clumped_val |= _mux->encode(fd);

	_tx_group->add_clumped(stamp, frame.can_id, mux_val, clumped_val);

	_last_stamp = stamp;
}

void tr_message::make_sig_assemblers() {
	for (const tr_signal& sig : _signals) {
		std::string_view atype = sig.agg_type();
		sig_asm sasm;

		if (atype == "LAST")
			sasm = make_sig_agg<sig_last>(sig);
		else if (atype == "AVG")
			sasm = make_sig_agg<sig_avg>(sig);
		else continue;

		_sig_asms.emplace_back(std::move(sasm));
	}
}

void tr_message::reset_sig_asms() {
	for (auto& sc : _sig_asms)
		sig_reset(sc);
}

tr_signal* tr_message::find_signal(const std::string& sig_name) {
	auto sig_it = std::find_if(_signals.begin(), _signals.end(), [&](const auto& s) { return s.name() == sig_name; });
	return sig_it == _signals.end() ? nullptr : &(*sig_it);
}

void tr_message::sig_agg_type(const std::string& sig_name, const std::string& agg_type) {
	if (auto sig = find_signal(sig_name); sig)
		sig->agg_type(agg_type);
}

void tr_message::sig_val_type(const std::string& sig_name, unsigned sig_ext_val_type) {
	if (auto sig = find_signal(sig_name); sig)
		sig->value_type(sig_ext_val_type);
}

void tr_message::add_signal(tr_signal sig) {
	_signals.push_back(std::move(sig));
}

void tr_message::add_muxer(tr_muxer mux) {
	_mux = std::move(mux);
}

bool vin_assembler::decode_some(const can::tr_message& msg, can_frame frame) {
	if (frame.can_id != _vin_msg_id)
		return false;
	auto old_bits = _cbits;
	uint64_t fd = *(const uint64_t*)frame.data;
	for (const auto& sig : msg.signals(fd)) {
		int chidx = vin_char(sig.name());
		if (chidx == 0) continue;
		_vin[chidx - 1] = char(sig.decode(fd));
		_cbits |= (1 << (chidx - 1));
	}
	return _cbits != old_bits;
}

int vin_assembler::vin_char(std::string_view sig_name) {
	if (sig_name.find_first_of("VIN") != 0) return 0;
	auto rb = sig_name.rbegin();
	while (rb != sig_name.rend() && std::isdigit(*rb)) ++rb;
	return std::stoi(sig_name.data() + std::distance(rb, sig_name.rend()));
}

frame_packet v2c_transcoder::transcode(can_time stamp, can_frame frame) {
	using namespace std::chrono;

	setup_timers(stamp);

	if (_last_update_tp + _update_freq <= stamp) {
		store_assembled(_last_update_tp + _update_freq);
		while (_last_update_tp + _update_freq <= stamp)
			_last_update_tp += _update_freq;
	}

	can_time frame_begin { seconds(_frame_packet.utc()) };
	can_time frame_end = frame_begin + _publish_freq;

	frame_packet rv {};

	if (stamp < frame_begin || stamp >= frame_end) {
		if (!_frame_packet.empty())
			rv = std::move(_frame_packet);
		_frame_packet.prepare(duration_cast<seconds>(stamp.time_since_epoch()).count());
	}

	auto mi = _msgs.find(frame.can_id);
	if (mi != _msgs.end()) {
		_vin.decode_some(mi->second, frame);
		mi->second.assemble(stamp, frame);
	}

	return rv;
}

void v2c_transcoder::setup_timers(can_time first_stamp) {
	using namespace std::chrono;

	if (_last_update_tp != can_time{})
		return;

	_frame_packet.prepare(duration_cast<seconds>(first_stamp.time_since_epoch()).count());
	_last_update_tp = first_stamp;

	for (auto& txg : _tx_groups)
		txg->time_begin(_last_update_tp);
}

void v2c_transcoder::store_assembled(can_time up_to) {
	for (auto& txg : _tx_groups)
		txg->try_publish(up_to, _frame_packet);
}

// helper methods for dbc_parser, to initialize the transcoder structures:

tr_message* v2c_transcoder::find_message(canid_t message_id) {
	auto msg_it = _msgs.find(message_id);
	return msg_it == _msgs.end() ? nullptr : &(msg_it->second);
}

void v2c_transcoder::assign_tx_group(
	const std::string& object_type, unsigned message_id, const std::string& tx_group
) {
	auto grp_it = std::find_if(_tx_groups.begin(), _tx_groups.end(),
		[&](const auto& g) { return g->name() == tx_group; });

	if (grp_it == _tx_groups.end())
		return;

	if (auto msg_ptr = find_message(message_id); msg_ptr)
		msg_ptr->assign_group(grp_it->get(), message_id);
}

void v2c_transcoder::add_signal(canid_t message_id, tr_signal sig) {
	if (auto msg_ptr = find_message(message_id); msg_ptr)
		msg_ptr->add_signal(std::move(sig));
}

void v2c_transcoder::add_muxer(canid_t message_id, tr_muxer mux) {
	if (auto msg_ptr = find_message(message_id); msg_ptr)
		msg_ptr->add_muxer(std::move(mux));
}

void v2c_transcoder::add_message(canid_t message_id, std::string_view message_name) {
	if (message_name == "VIN")
		_vin.vin_message_id(message_id);
	_msgs.try_emplace(message_id);
}

void v2c_transcoder::set_env_var(const std::string& name, int64_t ev_value) {
	using namespace std::chrono;

	if (name == "V2CTxTime") {
		_publish_freq = milliseconds(ev_value);
	}
	else if (name.ends_with("GroupTxFreq")) {
		_tx_groups.emplace_back(new tx_group(name, ev_value));

		auto ufreq = _update_freq / 1ms;
		ufreq = ufreq ? std::gcd(ufreq, ev_value) : ev_value;
		_update_freq = milliseconds(ufreq);
	}
}

void v2c_transcoder::set_sig_val_type(canid_t message_id, const std::string& sig_name, unsigned sig_ext_val_type) {
	if (auto msg_ptr = find_message(message_id); msg_ptr)
		msg_ptr->sig_val_type(sig_name, sig_ext_val_type);
}

void v2c_transcoder::set_sig_agg_type(canid_t message_id, const std::string& sig_name, const std::string& agg_type) {
	if (auto msg_ptr = find_message(message_id); msg_ptr)
		msg_ptr->sig_agg_type(sig_name, agg_type);
}

} // end namespace can
