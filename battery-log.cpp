#include <iostream>
#include <fstream>
#include <deque>
#include <chrono>
#include <thread>
#include <math.h>
#include <vector>
#include <functional>
#include <algorithm>
#include <format>
#include <numeric>

#include "battery.h"
#include "log.h"
#include "advancedConsole.h"

#define BATTERY_DESIGN_MAX "/sys/class/power_supply/BAT0/charge_full_design"
#define BATTERY_MAX "/sys/class/power_supply/BAT0/charge_full"
#define BATTERY_NOW "/sys/class/power_supply/BAT0/charge_now"

Battery battery;

using value_type = double;
using time_type = int64_t;

struct BatteryPoint = {
    value_type value;
    time_type time;
};

using LoopBuffer = LoopBufferT<BatteryPoint, 60 * 60 * 8>;
using DataLog = DataLogT<BatteryPoint, LoopBuffer>;

struct DataPlot {
    DataLog &log;
    Size size;
    value_type min, value_type max, value_type range, value_type range_inv;

    DataPlot(DataLog &log, const Size &size):log(log),size(size) {}

    int getY(const value_type &value) const {
        const int y = size.height - ((value - min) * range_inv * size.height);
        return ((y > size.height) ? size.height : ((y < 0) ? 0 : y)) + size.y;
    }

    void draw(const time_type &begin, const time_type &end) {
        min = log.min();
        max = log.max();
        range = log.range();
        range_inv = 1.0 / range;


    }
};

struct BatteryData {

    BatteryData():
        log(&data) {}

    LoopBuffer data;
    DataLog log;

    int64_t get_timestamp() {
        timeval tv;
        gettimeofday(&tv, nullptr);

        return tv.tv_sec + tv.tv_usec;
    }

    void poll() {
        log.push_back(get_timestamp(), battery.get_charge());
    }
};

int main(int argc, char **argv) {

    battery = Battery()
}
