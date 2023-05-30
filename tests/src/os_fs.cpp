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

METHOD_AS_TEST_CASE(testing::TestFsBasic<TestOsFs>::test, "SysFs");

// TODO: Test ChRootedFs
