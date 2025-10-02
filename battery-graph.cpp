#include <iostream>
#include <fstream>
#include <deque>
#include <chrono>
#include <thread>
#include <cmath>
#include <vector>
#include <functional>
#include <algorithm>
#include <format>
#include <numeric>

#include "battery.h"

const std::string BATTERY_DESIGN_MAX = "/sys/class/power_supply/BAT0/charge_full_design";
const std::string BATTERY_MAX = "/sys/class/power_supply/BAT0/charge_full";
const std::string BATTERY_NOW = "/sys/class/power_supply/BAT0/charge_now";
//const std::string BATTERY_DESIGN_MAX = "/sys/class/power_supply/BAT0/energy_full_design";
//const std::string BATTERY_MAX = "/sys/class/power_supply/BAT0/energy_full";
//const std::string BATTERY_NOW = "/sys/class/power_supply/BAT0/energy_now";
constexpr float interval = 0.1f;
std::vector<double> intervals = { (10./60.), (30./60.), 1, 5, 15, 60 };

using namespace std;
using value_type = double;
using clk = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<clk>;
using dur = std::chrono::duration<value_type>;
using deque_type = std::deque<value_type>;
using tuple_type = std::tuple<value_type, deque_type>;
using vector_type = std::vector<tuple_type>;

inline auto calculate_average(const deque_type& dq) {
    return std::reduce(dq.begin(), dq.end()) / dq.size();
}

inline auto calculate_range(const deque_type &dq) {
    const auto [min,max] = std::minmax_element(dq.begin(), dq.end());
    return *max - *min;
}

inline std::string format_hours(const value_type &hours_in, const bool &display_seconds = true) {
	const int c = 10;
	char buf[c];
	double t = hours_in;
	if (std::isinf(hours_in))
		t = 0.01;
        
	double hour = int(t);
	double minute = int((t - hour) * 60.0f);
    int seconds = abs(int((t - hour) * 3600.0f)) % 60;
	snprintf(&buf[0], c, "%02.lf:%02.lf", hour, fabs(minute));

    std::string ret(&buf[0]);
    if (display_seconds)
        ret += std::format(":{:02}", seconds);

	return ret;
}

inline std::string format_period(const value_type &period) {
    if (period < 1)
        return "sec";

    if (int(period) % 60 == 0)
        return "hour";

    return "min";
}

inline std::string format_interval(const value_type &interval) {
    const auto p_string = format_period(interval);

    if (interval < 1)
        return std::format("{:>3} {:>4}", int(interval * 60), p_string);

    if (int(interval) % 60 == 0)
        return std::format("{:>3} {:>4}", int(interval / 60), p_string);

    return std::format("{:>3} {:>4}", int(interval), p_string);
}

inline std::string format_battery(const value_type &charge, const bool &charging, const value_type &elapsed_time, const int &length) {
    std::string ret;
    ret += '[';
    int x_offset = 1;
    int x_size = length - 2;
    int final_pos = 0;
    for (int i = 0; i < x_size; i++) {
        value_type n = (value_type(i) * 100.0) / x_size;
        if (n < charge) {
            ret += '#';
            final_pos = i + x_offset;
        } else
            ret += ' ';
    }
    if (charging && int(elapsed_time)&1 && charge < 99.5)
    //if (charging)
        ret[final_pos] = ' ';
    ret += '}';
    return ret;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        std::vector<double> input;
        for (int i = 1; i < argc; i++) {
            double d;
            if (sscanf(argv[i], "%lf", &d) == 1)
                input.push_back(d);
        }
        if (input.size())
            intervals = input;
    }
    std::setvbuf(stdout, nullptr, _IOFBF, 1 << 14);

    vector_type values;
    for (auto &v : intervals)
        values.push_back(tuple_type(v,{}));

    Battery battery(BATTERY_MAX, BATTERY_DESIGN_MAX, BATTERY_NOW);

    value_type bat_offset = 0.00001;
    auto get_charge = [&battery,&bat_offset]() { return (value_type(battery.get_charge()) / (value_type(battery.get_capacity())+bat_offset)) * 100.0;};
    auto get_capacity = [&bat_offset]() { return value_type(100.0) + bat_offset; };

    auto start_battery = get_charge();
    int prev_state = -1;
    auto prev_battery = start_battery;
    auto current_battery = start_battery;

    tp start = clk::now();
    int samples = 0;
   
    const int bufsize=100;
    char filename[bufsize];
    snprintf(filename, bufsize, "/tmp/battery.%li.csv", time(nullptr));
    FILE *log = fopen(filename, "w");
    if (log)
	fprintf(log, "time, battery\n");

    while(true) {
        // Clear terminal
        printf("\033c");
        double elapsed = dur(clk::now() - start).count();
        bool real_data = false;

        if (fmod(fabs(elapsed), 1.0) < interval) {
            auto tmp_bat = get_charge();
            if (tmp_bat != current_battery) {
                current_battery = tmp_bat;
                real_data = true;
		if (log) {
		    fprintf(log, "%li, %lf\n", time(nullptr), current_battery);
		    fflush(log);
		}
            }
        }

        if (current_battery > 100)
            current_battery = 100.0;
        
        double difference = current_battery - start_battery;
        double diff_battery = fabs(difference);
    
        double diff_prev = current_battery - prev_battery;
        //bool charging = diff_prev > 0;
        bool charge_trend = diff_prev > 0;
        bool charging = prev_state == -1 ? charge_trend : prev_state;
        bool full = current_battery > 99.5;

        if (fabs(diff_prev) > 0.05) {
            if (prev_state != charge_trend) {
                if (prev_state != -1)
                    start_battery = current_battery;
                prev_state = charge_trend;
                for (auto &[t,deq] : values)
                    deq.clear();
                start = clk::now();
                samples = 0;
            } else {
                prev_battery = current_battery;
            }
        }

        //printf("prev_state: %i, charging: %i, diff_prev: %lf, prev_battery: %lf, current_battery: %lf, start_battery: %lf\n", prev_state, charging, diff_prev, prev_battery, current_battery, start_battery);

        double poll_freq = 60.0 / interval; // 60, 240
        double div = 60.0 * 60.0; // 3600, 
        double div2 = 60.0 * poll_freq;
        double hoursSinceStart = elapsed / div;
        double hoursLeft = start_battery;

        if (charging)
            hoursLeft = get_capacity() - current_battery;
        
        if (diff_battery > 0)
            hoursLeft = (hoursLeft / diff_battery) * hoursSinceStart;
        else
            hoursLeft = 0.0;
        
              //000.00% 000.00% 000.00% battery
        printf(" Current      Start    Diff   %s\n", format_battery(current_battery, charging, elapsed, 22).c_str());
        printf(" %6.2lf%%    %6.2lf%% %6.2lf%%   %s\n", current_battery, start_battery, diff_battery, full ? "Full" : (charging ? "Charging" : "Discharging"));
        printf(" Elapsed  Remaining\n");
        printf("%s   %s\n", format_hours(hoursSinceStart, true).c_str(), format_hours(hoursLeft).c_str());
              //10 second: 000.00% 000.00% 000.00% 000.00% 00:00:00
        printf("  Period      After    Diff   %i %s  Estimate\n", int(interval), format_period(interval).c_str());
        for (auto &[t,deq] : values) {
            double max_size = t * poll_freq;
            double size = deq.size();
            double sec_size = t * 60;
            double per_update = 1.0 / interval;

            bool half_full = max_size * 0.5 < size;
            bool has_data = size > 0 && calculate_range(deq) > 0.2;

            if (half_full || has_data) {
                auto prev = deq.back();

                if (real_data) {
                    deq.push_back(current_battery);
                } else {
                    //value_type gen = calculate_range(deq) / (size / per_update) * 2.0;
                    value_type YT = size * 1.4;
                    value_type gen = calculate_range(deq) / (YT);
                    if (!charging)
                        gen *= -1;
                    //deq.push_back(gen + current_battery);
                    value_type f = prev + gen;
                    deq.push_back(f);
                    //printf("%lf %lf %lf", gen, f, YT);
                }
            } else {
                deq.push_back(current_battery);
            }

            if (size > max_size)
                deq.pop_front();

            size = deq.size();

            auto bat_level = deq.back();
            auto mult = max_size / size;
            auto range = calculate_range(deq);
            //auto average = calculate_average(deq);
            auto norm = range / size * 60.0;
            auto remaining = bat_level;
            auto at_diff = range * mult;
            
            if (charging)
                remaining = get_capacity() - bat_level;

            auto remaining_time = (remaining / range) * (size/div2);

            if (range < 0.01)
                remaining_time = 0.0;
            
            auto at_time = bat_level - at_diff;
            
            if (charging)
                at_time = bat_level + at_diff;

            if (at_time < 0)
                at_time = 0.0;
            if (at_time > 100)
                at_time = get_capacity();

            at_diff = fabs(at_time - bat_level);

            auto formatted_hours = format_hours(remaining_time);

            //printf("%s    %6.2lf%% %6.2lf%% %6.2lf%% %6.2lf%% %s\n", format_interval(t).c_str(), at_time, range * mult, norm, range, formatted_hours.c_str());
            //, size, max_size, per_update, deq.back(), range, size / per_update, sec_size
            printf("%s    %6.2lf%% %6.2lf%% %6.2lf%%  %s\n", format_interval(t).c_str(), at_time, at_diff, norm, formatted_hours.c_str());
        }
        dur total_time = dur(samples * interval);
        this_thread::sleep_until(start + total_time);
        samples++;
        fflush(stdout);
    }

    return 0;
}
