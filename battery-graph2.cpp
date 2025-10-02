#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <functional>
#include <chrono>
#include <deque>

#define POWER_PATH "/sys/class/power_supply/BAT0/"

using value_type = double;
using time_type = double;
using clock_type = std::chrono::high_resolution_clock;
using time_point_type = std::chrono::time_point<clock_type>;
using duration_type = std::chrono::duration<time_type>;
using tuple_type = std::tuple<time_type, value_type>;
struct point_type;
using Point = point_type;
using deque_type = std::deque<point_type>;

const time_type time_precision = 1.0;

struct point_type : public tuple_type {
	point_type(const time_type &time, const value_type &value):
		tuple_type(time, value) {}
	
	const time_type &get_time() const { return std::get<0>(*this); }
	const value_type &get_value() const { return std::get<1>(*this); }
};

struct Interval : public deque_type {
	const time_type interval;

	Interval(const time_type &interval):
		interval(interval) {}

	duration_type get_duration() {
		const auto &start = this->back();
		const auto &end = this->front();

		return duration_type(end.get_time() - start.get_time());
	}

	::value_type get_range() {
		const auto &start = this->front();
		::value_type min = start.get_value(), max = start.get_value();

		for (const auto &point : *this) {
			const auto &value = point.get_value();

			if (min > value)
				min = value;
			if (max < value)
				max = value;
		}

		return max - min;
	}

	::value_type get_average() {
		::value_type sum = 0.0;

		for (const auto &point : *this) {
			sum += point.get_value();
		}

		return sum * (1.0 / this->size());
	}

	void fit_interval() {
		time_type min = this->back().get_time() - interval;

		while (this->front().get_time() < min) {
			this->pop_front();
		}
	}

	void add_point(const point_type &point) {
		this->push_back(point);
	}
};

struct Battery {
	const char *charge_now = POWER_PATH"charge_now";
	const char *charge_full = POWER_PATH"charge_full";
	const char *charge_max = POWER_PATH"charge_full_design";

	value_type capacity;

	std::vector<Interval> intervals;

	Battery() {
		capacity = get_capacity();
	}

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

	double get_normalized_charge() {
		return get_charge() * (1.0 / capacity);
	}

	point_type get_point() {
		auto tp = clock_type::now();
		auto dur = std::chrono::duration_cast<duration_type>(tp.time_since_epoch());
		
		return point_type(
			dur.count(),
			get_normalized_charge()
		);
	}

	void update_intervals() {
		auto point = get_point();

		for (auto &interval : intervals) {
			interval.add_point(point);

			if (interval.interval != 0)
				interval.fit_interval();
		}
	}
};

int main() {
	Battery battery;

	battery.intervals = {
		{0},{5},{10},{60},{120},{3600}
	};
}