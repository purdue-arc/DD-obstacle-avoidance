#pragma once

#include <chrono>

namespace bnchmk {
	class stopwatch {
		std::chrono::steady_clock clock;
		unsigned long long accumulated_time, last_time;

		unsigned long long get_running_duration() {
			return last_time ? (clock.now().time_since_epoch().count() - last_time) : 0;
		}
	public:
		stopwatch() : clock() {
			accumulated_time = 0;
			last_time = 0;
		}
		void start() {
			accumulated_time += get_running_duration();
			last_time = clock.now().time_since_epoch().count();
		}
		void pause() {
			accumulated_time += get_running_duration();
			last_time = 0;
		}
		void reset() {
			accumulated_time = 0;
			last_time = 0;
		}
		unsigned long long read() { // returns nanoseconds
			return accumulated_time + get_running_duration();
		}
		unsigned long long read_micro() {
			return read() / 1000ULL;
		}
		unsigned long long read_milli() {
			return read() / 1000000ULL;
		}
	};
}