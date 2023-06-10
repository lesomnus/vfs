#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include "testing/suites/file.hpp"

#include <vfs/impl/storage.hpp>

class TestVFile: public testing::TestFileFixture {
   public:
	std::shared_ptr<vfs::impl::Storage> make() {
		return std::make_shared<vfs::impl::VStorage>();
	}
};

METHOD_AS_TEST_CASE(testing::TestFile<TestVFile>::test, "VFile");
