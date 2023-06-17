#include <filesystem>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/utils.hpp"

namespace testing {
namespace suites {

class TestMountFixture {
   public:
	virtual std::shared_ptr<vfs::Fs> make_lhs() = 0;
	virtual std::shared_ptr<vfs::Fs> make_rhs() = 0;
};

template<std::derived_from<TestMountFixture> Fixture>
class TestMount {
   public:
	void test() {
		using namespace std::filesystem;

		Fixture fixture;

		auto lhs = fixture.make_lhs();
		auto rhs = fixture.make_rhs();

		lhs->create_directory("foo");
		rhs->create_directory("bar");

		lhs->mount("foo", *rhs->current_path("bar"));
		*lhs->open_write("foo/a") << "Lorem ipsum";

		std::string line;
		std::getline(*rhs->open_read("bar/a"), line);
		CHECK("Lorem ipsum" == line);

		lhs->unmount("foo");
		CHECK(not lhs->exists("foo/a"));

		std::getline(*rhs->open_read("bar/a"), line);
		CHECK("Lorem ipsum" == line);

		lhs->remove_all(lhs->current_path());
		rhs->remove_all(rhs->current_path());
	}
};

}  // namespace suites
}  // namespace testing

class TestMountOsFsOnOsFs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return testing::cd_temp_dir(*vfs::make_os_fs());
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return this->make_lhs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountOsFsOnOsFs>::test, "Mount OsFs on OsFs");

class TestMounVfsOnOsFs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return testing::cd_temp_dir(*vfs::make_os_fs());
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return testing::cd_temp_dir(*vfs::make_vfs());
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountOsFsOnOsFs>::test, "Mount Vfs on OsFs");

class TestMounOsFsOnVfs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return testing::cd_temp_dir(*vfs::make_vfs());
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return testing::cd_temp_dir(*vfs::make_os_fs());
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountOsFsOnOsFs>::test, "Mount OsFs on Vfs");

class TestMountVfsOnVfs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return testing::cd_temp_dir(*vfs::make_vfs());
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return this->make_lhs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountVfsOnVfs>::test, "Mount Vfs on Vfs");
