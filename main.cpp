#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

int main() {
	std::filesystem::remove("a");
	std::filesystem::remove("b");
	std::ofstream a("a");
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::ofstream b("b");

	std::cout << std::filesystem::last_write_time("a").time_since_epoch().count() << std::endl;
	std::cout << std::filesystem::last_write_time("b").time_since_epoch().count() << std::endl;
}
