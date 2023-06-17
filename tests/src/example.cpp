#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs.hpp>

#include "testing/utils.hpp"

void work(vfs::Fs& fs) {
	fs.create_directories("foo/bar");
	fs.create_symlink("foo/bar", "baz");

	*fs.open_write("baz/food") << "Royale with cheese";
}

TEST_CASE("example") {
	auto sandbox = vfs::make_vfs();
	work(*sandbox);

	std::string line;
	std::getline(*sandbox->open_read("foo/bar/food"), line);

	CHECK(line == "Royale with cheese");
}

TEST_CASE("mount") {
	namespace fs = std::filesystem;

	auto const sandbox = vfs::make_vfs();
	sandbox->create_directories("foo/bar");

	auto const os_fs = testing::cd_temp_dir(*vfs::make_os_fs());

	sandbox->mount("foo/bar", *os_fs);
	*sandbox->open_write("foo/bar/food") << "Royale with cheese";
	CHECK(sandbox->exists("foo/bar/food"));
	CHECK(fs::exists(os_fs->current_path() / "food"));

	{
		std::string line;
		std::getline(*sandbox->open_read("foo/bar/food"), line);
		CHECK(line == "Royale with cheese");
	}

	{
		std::ifstream food(os_fs->current_path() / "food");
		std::string   line;
		std::getline(food, line);
		CHECK(line == "Royale with cheese");
	}

	sandbox->unmount("foo/bar");
	CHECK(not sandbox->exists("foo/bar/food"));
	CHECK(fs::exists(os_fs->current_path() / "food"));
}
