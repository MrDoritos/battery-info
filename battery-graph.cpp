#include <iostream>
#include <fstream>
#include <deque>
#include <chrono>
#include <thread>
#include <cmath>

#define BATT_DESIGN_MAX "/sys/class/power_supply/BAT0/charge_full_design"
#define BATT_MAX "/sys/class/power_supply/BAT0/charge_full"
#define BATT_NOW "/sys/class/power_supply/BAT0/charge_now"
#define CHECK_INTERVAL 1     // Check every second
#define ONE_MIN 60           // 60 seconds
#define FIVE_MIN 300         // 300 seconds
#define FIFTEEN_MIN 900      // 900 seconds

using namespace std;
using clk = std::chrono::high_resolution_clock;
using tp = std::chrono::time_point<clk>;
using dur = std::chrono::duration<double, std::milli>;

// Function to get the battery level
int getBattery(const char* file) {
    ifstream ifile;
    ifile.open(file);
    if (!ifile.is_open())
        return -1;

    int o;
    ifile >> o;

    ifile.close();
    return o;
}

// Function to calculate the average
double calculateAverage(deque<int>& dq) {
    int sum = 0;
    for(auto val : dq)
        sum += val;
    return (double) sum / dq.size();
}

double calculateRange(deque<int> &dq) {
	int min = std::numeric_limits<int>::max();
	int max = std::numeric_limits<int>::min();
	for (auto val : dq) {
		if (val < min)
			min = val;
		if (val > max)
			max = val;
	}
	return max - min;
}

std::string format_hours(double hours_in) {
	const int c = 10;
	char buf[c];
	double t = hours_in;
	if (std::isinf(hours_in))
		t = 0.01;
	double hour = int(t);
	double minute = int((t - hour) * 60.0f);
	snprintf(&buf[0], c, "%.f:%02.f", hour, minute);
	return std::string(&buf[0]);
}

int main() {
    deque<int> oneMinBatt, fiveMinBatt, fifteenMinBatt;
    int maxBattery = getBattery(BATT_MAX);
    int startBattery = getBattery(BATT_NOW);
    int currentBattery = startBattery;
    
    tp start = clk::now();
    int samples = 0;
    
    while(true) {
        currentBattery = getBattery(BATT_NOW);
	
	int diffBattery = startBattery - currentBattery;
	double startRemaining = startBattery / double(maxBattery);
	double currentRemaining = currentBattery / double(maxBattery);
	double diffRemaining = diffBattery / double(maxBattery);
        
        oneMinBatt.push_back(currentBattery);
        fiveMinBatt.push_back(currentBattery);
        fifteenMinBatt.push_back(currentBattery);
	
        if(oneMinBatt.size() > ONE_MIN)
            oneMinBatt.pop_front();
        if(fiveMinBatt.size() > FIVE_MIN)
            fiveMinBatt.pop_front();
        if(fifteenMinBatt.size() > FIFTEEN_MIN)
            fifteenMinBatt.pop_front();
        
        double oneMinAvg = calculateAverage(oneMinBatt);
        double fiveMinAvg = calculateAverage(fiveMinBatt);
        double fifteenMinAvg = calculateAverage(fifteenMinBatt);

	double oneMinRg = calculateRange(oneMinBatt);
	double fiveMinRg = calculateRange(fiveMinBatt);
	double fifteenMinRg = calculateRange(fifteenMinBatt);

	bool charging = diffBattery < 0;

	//if (charging)
	//	diffRemaining = maxBattery - currentRemaining;
        
        double hoursSinceStart = (dur(clk::now() - start).count()) / (1000.0f * 60.0f * 60.0f);
        double hoursLeft1 = (currentBattery / oneMinRg) * (oneMinBatt.size()/3600.0f);
        double hoursLeft5 = (currentBattery / fiveMinRg) * (fiveMinBatt.size()/3600.0f);
        double hoursLeft15 = (currentBattery / fifteenMinRg) * (fifteenMinBatt.size()/3600.0f);
        double hoursLeft = (startRemaining / diffRemaining) * hoursSinceStart;
        
        // Clear terminal before printing new data
        cout << "\033c";       
        //cout << "Current Battery: " << currentBattery << endl;
        //cout << "Average over 1 min: " << oneMinAvg << " - Hours Left: " << hoursLeft1 << " hours" << endl;
        //cout << "Average over 5 min: " << fiveMinAvg << " - Hours Left: " << hoursLeft5 << " hours" << endl;
        //cout << "Average over 15 min: " << fifteenMinAvg << " - Hours Left: " << hoursLeft15 << " hours" << endl;
        printf("Battery now: %i, Battery start: %i, Battery difference: %i\n", currentBattery,
        								       startBattery,
        								       diffBattery);
        printf("%.1f%, %.1f%, %.1f%\n", currentRemaining * 100.0f,
        		       		startRemaining * 100.0f,
	        		        diffRemaining * 100.0f);
        printf("Time elapsed: %s:%02i, Time remaining: %s\n", format_hours(hoursSinceStart).c_str(), int(hoursSinceStart * 3600) % 60, format_hours(hoursLeft).c_str());
        printf("1 minute average: %.f, %s\n", oneMinRg, format_hours(hoursLeft1).c_str());
        printf("5 minute average: %.f, %s\n", fiveMinRg, format_hours(hoursLeft5).c_str());
        printf("15 minute average: %.f, %s\n", fifteenMinRg, format_hours(hoursLeft15).c_str());
        // current battery
        // max battery
        // fifteenminavg
        // time until 0%, currentBattery = 0
        // currentBattery / fifteenminavg = how many 15 min averages / 4 = how many hours
        
        //this_thread::sleep_for(chrono::seconds(CHECK_INTERVAL));
        dur total_time = dur(samples * CHECK_INTERVAL * 1000.0f);
        this_thread::sleep_until(start + total_time);
        samples++;
    }

    return 0;
}
