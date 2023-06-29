#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs.hpp>

#include "testing.hpp"

void work(vfs::Fs& fs) {
	fs.create_directories("foo/bar");
	fs.create_symlink("foo/bar", "baz");

	*fs.open_write("baz/food") << "Royale with cheese";
}

TEST_CASE("example") {
	auto sandbox = vfs::make_vfs();
	work(*sandbox);

	CHECK("Royale with cheese" == testing::read_all(*sandbox->open_read("foo/bar/food")));
}

TEST_CASE("mount") {
	namespace fs = std::filesystem;

	auto const sandbox = vfs::make_vfs();
	sandbox->create_directories("foo/bar");

	auto const os_fs = testing::cd_temp_dir(*vfs::make_os_fs());

	sandbox->mount("foo/bar", *os_fs, os_fs->current_path());
	*sandbox->open_write("foo/bar/food") << "Royale with cheese";
	CHECK(sandbox->exists("foo/bar/food"));
	CHECK(fs::exists(os_fs->current_path() / "food"));

	CHECK("Royale with cheese" == testing::read_all(*sandbox->open_read("foo/bar/food")));

	std::ifstream food(os_fs->current_path() / "food");
	CHECK("Royale with cheese" == testing::read_all(food));

	sandbox->unmount("foo/bar");
	CHECK(not sandbox->exists("foo/bar/food"));
	CHECK(fs::exists(os_fs->current_path() / "food"));
}
