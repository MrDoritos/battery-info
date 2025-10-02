#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <functional>
#include <chrono>

#define POWER_PATH "/sys/class/power_supply/BAT0/"

using point_type = double;
using time_type = double;
using clock_type = std::chrono::high_resolution_clock;
using time_point_type = std::chrono::time_point<clock_type>;
using duration_type = std::chrono::duration<time_type>;

struct Interval {
	
};

struct Battery {
	const char *charge_now = POWER_PATH"charge_now";
	const char *charge_full = POWER_PATH"charge_full";
	const char *charge_max = POWER_PATH"charge_full_design";

	std::vector<Interval> intervals;

	static double read_file(const char *filepath) {
		std::ifstream file(filepath);

		assert(file.is_open() && "Could not open file\n");

		double n;
		file >> n;
		return n;
	}

	double get_capacity() { return read_file(charge_full); }

	double get_design_capacity() { return read_file(charge_max); }

	double get_charge() { return read_file(charge_now); }
};