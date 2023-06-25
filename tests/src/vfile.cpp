#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/vfile.hpp>

#include "testing/suites/file.hpp"

class TestVFile: public testing::suites::TestFileFixture {
   public:
	std::shared_ptr<vfs::impl::Directory> make() {
		return std::make_shared<vfs::impl::VDirectory>(0, 0);
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestFile<TestVFile>::test, "VFile");
