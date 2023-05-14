#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/suites/fs.hpp"

class TestVfs: public testing::TestFsFixture {
   public:
	std::shared_ptr<vfs::Fs> make() override {
		return vfs::make_sys_fs();
	}
};

METHOD_AS_TEST_CASE(testing::TestFsBasic<TestVfs>::test, "SysFs");
