#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#define BATT_MAX "/sys/class/power_supply/BAT0/charge_full"
#define BATT_NOW "/sys/class/power_supply/BAT0/charge_now"

using namespace std;

int getint(const char* file) {
	ifstream ifile;
	ifile.open(file);
	if (!ifile.is_open())
		return -1;
	
	int o;
	ifile >> o;
	
	ifile.close();
	return o;
}

int main() {
	double perc = 0.0d;
	
	double max = getint(BATT_MAX);
	double now = getint(BATT_NOW);
	
	perc = (now / max) * 100.0d;
	
	printf("%i\%\n", int(perc));
}
