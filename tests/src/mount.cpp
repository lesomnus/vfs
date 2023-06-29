#include <filesystem>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing.hpp"

namespace fs = std::filesystem;

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

		SECTION("mount directory") {
			lhs->create_directory("foo");
			rhs->create_directory("bar");
			REQUIRE(lhs->is_directory("foo"));
			REQUIRE(rhs->is_directory("bar"));

			*lhs->open_write("foo/a") << testing::QuoteA;
			lhs->mount("foo", *rhs, "bar");
			CHECK(not lhs->exists("foo/a"));
			CHECK(not rhs->exists("bar/a"));

			*lhs->open_write("foo/b") << testing::QuoteB;
			CHECK(testing::QuoteB == testing::read_all(*lhs->open_read("foo/b")));
			CHECK(testing::QuoteB == testing::read_all(*rhs->open_read("bar/b")));

			lhs->unmount("foo");
			CHECK(not lhs->exists("foo/b"));
			CHECK(testing::QuoteA == testing::read_all(*lhs->open_read("foo/a")));
			CHECK(testing::QuoteB == testing::read_all(*rhs->open_read("bar/b")));
		}

		SECTION("cannot remove mountpoint") {
			SECTION("regular file") {
				*lhs->open_write("foo") << testing::QuoteA;
				*rhs->open_write("bar") << testing::QuoteB;
				lhs->mount("foo", *rhs, "bar");

				std::error_code ec;
				lhs->remove("foo", ec);
				CHECK(std::errc::device_or_resource_busy == ec);
			}

			SECTION("directory") {
				lhs->create_directory("foo");
				rhs->create_directory("bar");
				lhs->mount("foo", *rhs, "bar");

				std::error_code ec;
				lhs->remove("foo", ec);
				CHECK(std::errc::device_or_resource_busy == ec);
			}

			SECTION("directory which contains mount point") {
				lhs->create_directories("foo/bar");
				rhs->create_directory("bar");
				lhs->mount("foo/bar", *rhs, "bar");

				std::error_code ec;
				lhs->remove_all("foo", ec);
				CHECK(std::errc::device_or_resource_busy == ec);
			}
		}

		SECTION("operation on the attached directory") {
			// Original.
			// /
			// + a/
			//   + foo/
			//     + x
			//   + bar/
			//     + baz/
			//     + y
			lhs->create_directories("a/foo");
			lhs->create_directories("a/bar/baz");
			*lhs->open_write("a/foo/x") << testing::QuoteA;
			*lhs->open_write("a/bar/y") << testing::QuoteB;

			// Attached.
			// /
			// + b/
			//   + foo/
			//     + x
			//   + bar/
			rhs->create_directories("b/foo");
			rhs->create_directories("b/bar");
			*rhs->open_write("x") << testing::QuoteC;

			lhs->create_directory("b");
			lhs->mount("b", *rhs, "b");

			SECTION("copy to") {
				using opts = fs::copy_options;

				lhs->copy("a", "b", opts::recursive | opts::overwrite_existing);

				CHECK(rhs->is_directory("b/foo"));
				CHECK(testing::QuoteA == testing::read_all(*rhs->open_read("b/foo/x")));
				CHECK(rhs->is_directory("b/bar/baz"));
				CHECK(testing::QuoteB == testing::read_all(*rhs->open_read("b/bar/y")));
			}

			SECTION("rename to") {
				rhs->remove_all("b/foo");
				rhs->remove_all("b/bar");
				REQUIRE(rhs->is_empty("b"));

				// `rename` first removes the target and then links the original node to the target.
				// So the target, "b", is a mount point, it fails.
				std::error_code ec;
				lhs->rename("a", "b", ec);
				CHECK(std::errc::device_or_resource_busy == ec);
			}

			SECTION("rename into") {
				lhs->rename("a/bar", "b/bar");
				CHECK(not lhs->exists("a/bar"));
				CHECK(rhs->is_directory("b/bar/baz"));
				CHECK(testing::QuoteB == testing::read_all(*rhs->open_read("b/bar/y")));
			}
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
