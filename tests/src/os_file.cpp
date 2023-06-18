#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/os_file.hpp>

#include "testing/suites/file.hpp"

class TestOsFile: public testing::suites::TestFileFixture {
   public:
	std::shared_ptr<vfs::impl::Directory> make() {
		return std::make_shared<vfs::impl::TempDirectory>();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestFile<TestOsFile>::test, "OsFile");
