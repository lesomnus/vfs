#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/suites/fs.hpp"

class TestOsFs: public testing::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		return vfs::make_os_fs();
	}
};

METHOD_AS_TEST_CASE(testing::TestFsBasic<TestOsFs>::test, "OsFs");

class TestChRootedOsFs: public testing::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		auto fs = vfs::make_vfs();

		auto const tmp = fs->temp_directory_path();
		fs->create_directories(tmp / "tmp");
		REQUIRE(fs->is_directory(tmp / "tmp"));

		return fs->change_root(tmp);
	}
};

METHOD_AS_TEST_CASE(testing::TestFsBasic<TestChRootedOsFs>::test, "OsFs with chroot");
