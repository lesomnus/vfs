#include <catch2/catch_template_test_macros.hpp>

#include <vfs/directory_entry.hpp>
#include <vfs/fs.hpp>

TEST_CASE("directory_entry") {
	auto fs = vfs::make_vfs();
	fs->create_directory("foo");
	fs->open_write("bar");

	REQUIRE(fs->is_directory("foo"));
	REQUIRE(fs->is_regular_file("bar"));
	REQUIRE(not fs->exists("baz"));

	SECTION("::assign") {
		vfs::directory_entry entry(*fs, "baz");
		CHECK(not entry.exists());

		entry.assign("foo");
		CHECK(entry.is_directory());

		entry.assign("bar");
		CHECK(entry.is_regular_file());
	}

	SECTION("::refresh") {
		vfs::directory_entry entry(*fs, "baz");
		CHECK(not entry.exists());

		fs->open_write("baz");
		entry.refresh();
		CHECK(entry.is_regular_file());
	}

	SECTION("::replace_filename") {
		vfs::directory_entry entry(*fs, "baz");
		CHECK(not entry.exists());

		entry.replace_filename("foo");
		CHECK(entry.is_directory());

		entry.replace_filename("bar");
		CHECK(entry.is_regular_file());
	}

	SECTION("::path") {
		vfs::directory_entry entry(*fs, "baz");
		CHECK("baz" == entry.path());

		std::filesystem::path const p = entry;
		CHECK("baz" == p);
	}

	SECTION("::exists") {
		vfs::directory_entry entry(*fs, "foo");
		CHECK(entry.exists());
	}

	SECTION("::is_directory") {
		vfs::directory_entry entry(*fs, "foo");
		CHECK(entry.is_directory());
	}

	SECTION("::is_regular_file") {
		vfs::directory_entry entry(*fs, "bar");
		CHECK(entry.is_regular_file());
	}

	SECTION("comparison") {
		vfs::directory_entry entry1(*fs, "foo");
		vfs::directory_entry entry2(*fs, "bar");
		CHECK(entry1 != entry2);
		CHECK(entry1 > entry2);
		CHECK(entry2 < entry1);
		CHECK(not(entry1 == entry2));
		CHECK(not(entry1 < entry2));
		CHECK(not(entry1 <= entry2));

		auto const order = entry1 <=> entry2;
		CHECK(order != 0);
		CHECK(order > 0);
		CHECK(not(order < 0));

		entry2.assign("foo");
		CHECK(entry1 == entry2);
		CHECK(entry1 >= entry2);
		CHECK(entry2 <= entry1);
		CHECK(not(entry1 != entry2));
		CHECK(not(entry1 < entry2));
		CHECK(not(entry1 > entry2));
	}
}
