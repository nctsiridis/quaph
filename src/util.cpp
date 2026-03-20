#include "../include/util.hpp"

std::string tm_to_string(const std::tm& tm) {
	std::ostringstream oss;
	oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return oss.str();
}

std::string get_timestamp() {
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tm = *std::localtime(&t);
	return tm_to_string(tm);
}

bool in_region(float x, float y, float x_top, float x_bottom, float y_right, float y_left) {
	return x <= x_top && x >= x_bottom && y <= y_right && y >= y_left;
}
