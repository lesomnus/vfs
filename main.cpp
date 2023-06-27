#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
	fs::create_directories("a/o");
	fs::create_directories("b/p");

	fs::rename("a", "b");

	return 0;
}
