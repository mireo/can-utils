#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <random>
#include <bit>

#include "dbc/dbc_parser.h"
#include "v2c/v2c_transcoder.h"

std::string read_file(const std::string& dbc_path) {
	std::ifstream dbc_content(dbc_path);
	std::ostringstream ss;
	ss << dbc_content.rdbuf();
	return ss.str();
}

can_frame generate_frame() {
	static std::default_random_engine generator; 
	static std::uniform_int_distribution<int64_t> frame_data_dist(LLONG_MIN, LLONG_MAX); // random 64-bit signed integer
	static std::uniform_int_distribution<int64_t> can_id_dist(1, 7); // 7 is the highest can_id in example.dbc

	can_frame frame;
	*(int64_t*)frame.data = frame_data_dist(generator);
	frame.can_id = can_id_dist(generator); 

	return frame;
}

void print_frames(const can::frame_packet& fp, can::v2c_transcoder& transcoder, int32_t frame_counter) {
	using namespace std::chrono;

	std::cout << "New frame_packet (from " << frame_counter << " frames):" << std::endl;

	for (const auto& [ts, frame] : fp) {
		auto frame_data = std::bit_cast<int64_t>(frame.data);
		auto t = duration_cast<milliseconds>(ts.time_since_epoch()).count() / 1000.0;

		std::cout << std::fixed
			<< " can_frame at t: " << t << "s, can_id: " << frame.can_id << std::endl;

		auto msg = transcoder.find_message(frame.can_id);
		for (const auto& sig : msg->signals(frame_data))
			std::cout << "  " << sig.name() << ": " << (int64_t)sig.decode(frame_data) << std::endl;
	}
}

int main() {
	can::v2c_transcoder transcoder;

	auto start = std::chrono::system_clock::now();

	bool parsed = can::parse_dbc(read_file("example/example.dbc"), std::ref(transcoder));
	if (!parsed)
		return 1;

	auto end = std::chrono::system_clock::now();

	std::cout << "Parsed DBC in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

	int32_t frame_counter = 0;

	while (true) {
		// Simulate receiving a frame from CAN bus
		can_frame frame = generate_frame();
		frame_counter++;

		auto fp = transcoder.transcode(std::chrono::system_clock::now(), frame);
		
		if (fp.empty())
			continue;
		
		// Send the aggregated frame_packet to a remote server, store it locally, or process it.
		// This example just prints it to stdout.

		print_frames(fp, transcoder, frame_counter);
		frame_counter = 0;
	}

	return 0;
}
