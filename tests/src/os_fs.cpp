#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/suites/fs.hpp"
#include "testing/utils.hpp"

class TestOsFs: public testing::suites::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		return vfs::make_os_fs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestFsBasic<TestOsFs>::test, "OsFs");

class TestChRootedOsFs: public testing::suites::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		auto fs = testing::cd_temp_dir(*vfs::make_os_fs());

		fs = fs->change_root(fs->current_path());
		fs->create_directories(fs->temp_directory_path());
		return fs;
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestFsBasic<TestChRootedOsFs>::test, "OsFs with chroot");
