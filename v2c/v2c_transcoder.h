#pragma once

#include <type_traits>
#include <unordered_map>
#include <vector>
#include <ranges>
#include <algorithm>
#include <utility>

#include "any/any.h"
#include "can/can_codec.h"
#include "can/frame_packet.h"
#include "dbc/dbc_parser.h"
#include "dbc/parser_template.h"

namespace can {

constexpr struct assemble_cpo {
	using type_erased_signature_t = uint64_t(mireo::this_&, int64_t, uint64_t);

	template<typename T> requires mireo::tag_invocable<assemble_cpo, T&, uint64_t, uint64_t>
	uint64_t operator()(T& x, int64_t mux_val, uint64_t fd) const {
		return mireo::tag_invoke(*this, x, mux_val, fd);
	}
} sig_assemble;

constexpr struct reset_cpo {
	using type_erased_signature_t = void(mireo::this_&);

	template<typename T> requires mireo::tag_invocable<reset_cpo, T&>
	void operator()(T& x) const {
		return mireo::tag_invoke(*this, x);
	}
} sig_reset;

using sig_asm = mireo::any<sig_assemble, sig_reset>;

class tr_signal {
	std::string _name;
	sig_codec _codec;

	std::string _agg_type = "LAST";
	val_type_t _val_type = i64;
	std::optional<int64_t> _mux_val; 
public:
	tr_signal(std::string name, sig_codec codec, std::optional<int64_t> mux_val)
		: _name(std::move(name)), _codec(codec), _mux_val(mux_val)
	{}

	const std::string& name() const { return _name; }
	std::optional<int64_t> mux_val() const { return _mux_val; }

	bool is_active(uint64_t frame_mux_val) const {
		return !_mux_val || _mux_val == frame_mux_val;
	}
	
	std::string_view agg_type() const { return _agg_type; }
	void agg_type(const std::string& agg_type) { _agg_type = agg_type; }
	val_type_t value_type() const { return _val_type; }

	void value_type(unsigned vt) {
		_val_type = val_type_t(vt);
		if (_val_type == i64 && _codec.sign_type() == '+')
			_val_type = u64;
	}

	uint64_t decode(uint64_t data) const {
		return _codec((uint8_t*)&data);
	}

	uint64_t encode(uint64_t raw) const {
		uint64_t rv = 0;
		_codec(raw, (void*)&rv);
		return rv;
	}
};

class tr_muxer {
	sig_codec _codec;
public:
	tr_muxer(sig_codec codec) : _codec(codec) {}

	uint64_t decode(uint64_t data) const {
		return _codec((uint8_t*)&data);
	}
	
	uint64_t encode(uint64_t raw) const {
		uint64_t rv = 0;
		_codec(raw, (void*)&rv);
		return rv;
	}
};

class tr_message;

class tx_group {
	struct stamped_msg {
		can_time stamp;
		canid_t message_id;
		int64_t message_mux;
		uint64_t mdata;
	};

	std::string _name;
	std::chrono::milliseconds _assemble_freq;
	can_time _group_origin;

	std::vector<stamped_msg> _msg_clumps;

	friend class tr_message;
public:
	tx_group(std::string_view name, uint32_t assemble_freq) :
		_name(name), _assemble_freq(assemble_freq)
	{}

	std::string_view name() const { return _name; }
	void time_begin(can_time tp) { _group_origin = tp; }
	void try_publish(can_time up_to, frame_packet& fp);

private:
	void publish(can_time tp, frame_packet& fp);
	bool all_collected() const;
	void assign(canid_t message_id, int64_t message_mux);
	bool within_interval(can_time stamp) const;
	void add_clumped(can_time stamp, canid_t message_id, int64_t message_mux, uint64_t cval);
};

class tr_message {
	std::vector<tr_signal> _signals;
	std::optional<tr_muxer> _mux;

	std::vector<sig_asm> _sig_asms;
	tx_group* _tx_group = nullptr;
	can_time _last_stamp;

public:
	void assign_group(tx_group* txg, uint32_t message_id);
	void assemble(can_time stamp, can_frame frame);

	void sig_agg_type(const std::string& sig_name, const std::string& agg_type);
	void sig_val_type(const std::string& sig_name, unsigned sig_ext_val_type);
	void add_signal(tr_signal sig);
	void add_muxer(tr_muxer mux);

	auto signals(uint64_t fd) const {
		uint64_t frame_mux = _mux.has_value() ? _mux->decode(fd) : -1;
		return _signals | std::ranges::views::filter([frame_mux](const auto& sig) {
			return sig.is_active(frame_mux);
		});
	}
private:
	void make_sig_assemblers();
	void reset_sig_asms();
	tr_signal* find_signal(const std::string& sig_name);
	std::vector<uint64_t> distinct_mux_vals() const;
};

class vin_assembler {
	static constexpr size_t vin_len = 17; // industry standard
	uint32_t _vin_msg_id;
	uint32_t _cbits = 0;
	char _vin[vin_len];
public:
	bool empty() const {
		return _cbits != ((1 << vin_len) - 1);
	}
	std::string value() const {
		return empty() ? std::string{} : std::string{ _vin, _vin + vin_len };
	}
	void vin_message_id(uint32_t id) {
		_vin_msg_id = id;
	}

	bool decode_some(const can::tr_message& msg, can_frame frame);
private:
	static int vin_char(std::string_view sig_name);
};

class v2c_transcoder {
	std::chrono::milliseconds _publish_freq;
	std::chrono::milliseconds _update_freq{ 0 }; // gcd of all tx_groups' freqs

	std::unordered_map<canid_t, tr_message> _msgs;
	std::vector<std::unique_ptr<tx_group>> _tx_groups;
	vin_assembler _vin;

	frame_packet _frame_packet;
	can_time _last_update_tp;
public:
	frame_packet transcode(can_time stamp, can_frame frame);
	std::string vin() const { return _vin.value(); }

	void assign_tx_group(const std::string& object_type, unsigned message_id, const std::string& tx_group);
	void add_signal(canid_t message_id, tr_signal sig);
	void add_muxer(canid_t message_id, tr_muxer mux);
	void add_message(canid_t message_id, std::string_view message_name);

	void set_env_var(const std::string& name, int64_t ev_value);
	void set_sig_val_type(canid_t message_id, const std::string& sig_name, unsigned sig_ext_val_type);
	void set_sig_agg_type(canid_t message_id, const std::string& sig_name, const std::string& agg_type);
	
	void setup_timers(can_time first_stamp);
	void store_assembled(can_time up_to);
	tr_message* find_message(canid_t message_id);
};

// tag-invokes used by dbc_parser.cpp

inline void tag_invoke(
	def_sg_cpo, v2c_transcoder& this_,
	uint32_t message_id, std::optional<unsigned> sg_mux_switch_val, std::string sg_name,
	unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
	double sg_factor, double sg_offset, double sg_min, double /*sg_max*/,
	std::string sg_unit, std::vector<size_t> rec_ords
) {
	sig_codec codec{ sg_start_bit, sg_size, sg_byte_order, sg_sign };
	tr_signal sig{ sg_name, codec, std::optional<int64_t>(sg_mux_switch_val) };
	this_.add_signal(message_id, std::move(sig));
}

inline void tag_invoke(
	def_sg_mux_cpo, v2c_transcoder& this_,
	uint32_t message_id, std::string sg_name,
	unsigned sg_start_bit, unsigned sg_size, char sg_byte_order, char sg_sign,
	std::string sg_unit, std::vector<size_t> rec_ords
) {
	sig_codec codec{ sg_start_bit, sg_size, sg_byte_order, sg_sign };
	tr_muxer mux{ codec };
	this_.add_muxer(message_id, std::move(mux));
}

inline void tag_invoke(
	def_bo_cpo, v2c_transcoder& this_,
	uint32_t message_id, std::string msg_name, size_t msg_size, size_t transmitter_ord
) {
	this_.add_message(message_id, std::move(msg_name));
}

inline void tag_invoke(
	def_ev_cpo, v2c_transcoder& this_,
	std::string name, unsigned type, double ev_min, double ev_max,
	std::string unit, double initial, unsigned ev_id,
	std::string access_type, std::vector<size_t> access_nodes_ords
) {
	this_.set_env_var(name, initial);
}

inline void tag_invoke(
	def_ba_cpo, v2c_transcoder& this_,
	std::string attr_name, std::string object_type, std::string object_name,
	size_t bu_id, unsigned message_id,
	std::variant<int32_t, double, std::string> attr_val
) {
	if (attr_name == "AggType" && attr_val.index() == 2)
		this_.set_sig_agg_type(message_id, object_name, std::get<std::string>(attr_val));

	if (attr_name == "TxGroupFreq" && object_type == "BO_")
		this_.assign_tx_group(object_type, message_id, std::get<std::string>(attr_val));
}

inline void tag_invoke(
	def_sig_valtype_cpo, v2c_transcoder& this_,
	unsigned message_id, std::string sig_name, unsigned sig_ext_val_type
) {
	this_.set_sig_val_type(message_id, sig_name, sig_ext_val_type);
}

} // end namesapce can
