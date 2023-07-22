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
