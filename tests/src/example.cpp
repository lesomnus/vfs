#include <memory>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs.hpp>

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
