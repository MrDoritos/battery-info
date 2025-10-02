#include <string>
#include <fstream>
#include <cassert>

int read_number(const std::string &file_path) {
    std::ifstream file(file_path);

    assert(file.is_open() && "File does not exist\n");

    int n;
    file >> n;
    return n;
}

struct Battery {
    Battery(const std::string &capacity_file, const std::string &design_capacity_file, const std::string &current_charge_file)
            :current_charge_file(current_charge_file) {
        capacity = read_number(capacity_file);
        design_capacity = read_number(capacity_file);
    }

    Battery() {}

    std::string current_charge_file;

    double capacity;
    double design_capacity;

    double get_capacity() {
        return capacity;
    }

    double get_design_capacity() {
        return design_capacity;
    }

    double get_charge() {
        return read_number(current_charge_file);
    }
};
