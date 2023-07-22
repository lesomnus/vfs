#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/suites/fs.hpp"

class TestUnionFs: public testing::suites::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		return vfs::make_union_fs(*vfs::make_mem_fs(), *vfs::make_mem_fs());
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestFsBasic<TestUnionFs>::test, "UnionFs");

class TestChRootedVfs: public testing::suites::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		auto fs = vfs::make_mem_fs();
		fs->create_directories("root_/tmp");
		REQUIRE(fs->is_directory("root_/tmp"));

		return fs->change_root("root_");
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestFsBasic<TestChRootedVfs>::test, "UnionFs with chroot");

TEST_CASE("UnionFs") {
	auto upper = vfs::make_mem_fs();
	auto lower = vfs::make_mem_fs();
	auto root  = vfs::make_union_fs(*upper, *lower);

	SECTION("::rename") {
		SECTION("file on lower must be preserved") {
			upper->create_directory("foo");
			lower->create_directory("bar");
			*lower->open_write("bar/baz") << testing::QuoteA;
			REQUIRE(upper->is_directory("foo"));
			REQUIRE(lower->is_regular_file("bar/baz"));
			CHECK(root->is_directory("foo"));
			CHECK(root->is_regular_file("bar/baz"));

			root->rename("bar/baz", "foo/qux");
			CHECK(upper->is_regular_file("foo/qux"));
			CHECK(lower->is_regular_file("bar/baz"));
			CHECK(root->is_regular_file("foo/qux"));
			CHECK(not root->exists("bar/baz"));

			CHECK(testing::QuoteA == testing::read_all(*root->open_read("foo/qux")));
		}
	}
}
