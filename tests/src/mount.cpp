#include <filesystem>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/utils.hpp"

namespace fs = std::filesystem;

TEST_CASE("mount") {
	SECTION("os_fs on os_fs") {
		auto fs1 = testing::cd_temp_dir(*vfs::make_os_fs());
		auto fs2 = testing::cd_temp_dir(*vfs::make_os_fs());

		fs1->create_directory("foo");
		fs2->create_directory("bar");

		fs1->mount("foo", *fs2->current_path("bar"));
		*fs1->open_write("foo/a") << "Lorem ipsum";

		std::string line;
		std::getline(*fs2->open_read("bar/a"), line);
		CHECK("Lorem ipsum" == line);

		fs1->unmount("foo");
		CHECK(not fs1->exists("foo/a"));

		std::getline(*fs2->open_read("bar/a"), line);
		CHECK("Lorem ipsum" == line);

		fs::remove_all(fs1->current_path());
		fs::remove_all(fs2->current_path());
	}

	SECTION("vfs on vfs") {
		auto fs1 = vfs::make_vfs();
		auto fs2 = vfs::make_vfs();

		fs1->create_directory("foo");
		fs2->create_directory("bar");

		fs1->mount("foo", *fs2->current_path("bar"));
		*fs1->open_write("foo/a") << "Lorem ipsum";

		std::string line;
		std::getline(*fs2->open_read("bar/a"), line);
		CHECK("Lorem ipsum" == line);

		fs1->unmount("foo");
		CHECK(not fs1->exists("foo/a"));

		std::getline(*fs2->open_read("bar/a"), line);
		CHECK("Lorem ipsum" == line);
	}
}
