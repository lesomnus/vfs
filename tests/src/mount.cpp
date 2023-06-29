#include <filesystem>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing.hpp"

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

		std::shared_ptr<vfs::Fs> lhs = testing::cd_temp_dir(*fixture.make_lhs());
		std::shared_ptr<vfs::Fs> rhs = testing::cd_temp_dir(*fixture.make_rhs());

		std::string line;

		SECTION("mount regular file") {
			*lhs->open_write("foo") << testing::QuoteA;
			*rhs->open_write("bar") << testing::QuoteB;
			REQUIRE(lhs->is_regular_file("foo"));
			REQUIRE(rhs->is_regular_file("bar"));

			lhs->mount("foo", *rhs, "bar");
			CHECK(testing::QuoteB == testing::read_all(*lhs->open_read("foo")));

			lhs->unmount("foo");
			CHECK(testing::QuoteA == testing::read_all(*lhs->open_read("foo")));
		}

		// SECTION("access attached directory") {
		// 	lhs->create_directory("foo");
		// 	rhs->create_directory("bar");

		// 	lhs->mount("foo", *rhs->current_path("bar"));
		// 	*lhs->open_write("foo/a") << testing::QuoteA;

		// 	std::string line;
		// 	std::getline(*rhs->open_read("bar/a"), line);
		// 	CHECK(testing::QuoteA == line);

		// 	lhs->unmount("foo");
		// 	CHECK(not lhs->exists("foo/a"));

		// 	std::getline(*rhs->open_read("bar/a"), line);
		// 	CHECK(testing::QuoteA == line);

		// 	lhs->remove_all(lhs->current_path());
		// 	rhs->remove_all(rhs->current_path());
		// }

		// SECTION("unmount restores mountpoint") {
		// 	lhs->create_directory("foo");
		// 	rhs->create_directory("bar");

		// 	*lhs->open_write("foo/a") << testing::QuoteA;
		// 	lhs->mount("foo", *rhs->current_path("bar"));
		// 	CHECK(not lhs->exists("foo/a"));

		// 	lhs->unmount("foo");
		// 	CHECK(lhs->exists("foo/a"));

		// 	std::string line;
		// 	std::getline(*lhs->open_read("foo/a"), line);
		// 	CHECK(testing::QuoteA == line);
		// }

		SECTION("rename to attached directory") {
		}
	}
};

}  // namespace suites
}  // namespace testing

class TestMountOsFsOnOsFs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return vfs::make_os_fs();
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return vfs::make_os_fs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountOsFsOnOsFs>::test, "Mount OsFs on OsFs");

class TestMounVfsOnOsFs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return vfs::make_os_fs();
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return vfs::make_vfs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountOsFsOnOsFs>::test, "Mount Vfs on OsFs");

class TestMounOsFsOnVfs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return vfs::make_vfs();
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return vfs::make_os_fs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountOsFsOnOsFs>::test, "Mount OsFs on Vfs");

class TestMountVfsOnVfs: public testing::suites::TestMountFixture {
   public:
	std::shared_ptr<vfs::Fs> make_lhs() override {
		return vfs::make_vfs();
	}

	std::shared_ptr<vfs::Fs> make_rhs() override {
		return vfs::make_vfs();
	}
};

METHOD_AS_TEST_CASE(testing::suites::TestMount<TestMountVfsOnVfs>::test, "Mount Vfs on Vfs");
