#include <chrono>
#include <concepts>
#include <memory>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

#include "testing/utils.hpp"

namespace fs = std::filesystem;

class TestCopyFixture {
   public:
	virtual std::shared_ptr<vfs::Fs> make_src_fs() = 0;
	virtual std::shared_ptr<vfs::Fs> make_dst_fs() = 0;
};

template<std::derived_from<TestCopyFixture> Fixture>
class TestCopy {
   public:
	void test() {
		Fixture fixture;

		std::shared_ptr<vfs::Fs> src = testing::cd_temp_dir(*fixture.make_src_fs());
		std::shared_ptr<vfs::Fs> dst = testing::cd_temp_dir(*fixture.make_dst_fs());

		// /
		// + foo/
		//   + bar/
		//     + baz/
		//   + qux/
		//   + dog
		//   + cat -> ./dog
		src->create_directories("foo/bar/baz");
		src->create_directories("foo/qux");
		*src->open_write("foo/dog") << "woof";
		src->create_symlink("./dog", "foo/cat");

		SECTION("regular file") {
			SECTION("without option") {
				src->copy("foo/dog", *dst, "dog");
				REQUIRE(dst->is_regular_file("dog"));

				std::string content;
				std::getline(*dst->open_read("dog"), content);
				CHECK("woof" == content);
			}

			SECTION("with option directory_only") {
				src->copy("foo/dog", *dst, "dog", fs::copy_options::directories_only);
				CHECK(not dst->exists("dog"));
			}

			SECTION("to directory") {
				dst->create_directory("foo");
				REQUIRE(dst->is_directory("foo"));

				src->copy("foo/dog", *dst, "foo");
				REQUIRE(dst->is_regular_file("foo/dog"));

				std::string content;
				std::getline(*dst->open_read("foo/dog"), content);
				CHECK("woof" == content);
			}

			SECTION("to existing regular file") {
				*dst->open_write("dog") << "howl";
				REQUIRE(dst->is_regular_file("dog"));

				SECTION("without option") {
					std::error_code ec;
					src->copy("foo/dog", *dst, "dog", ec);
					CHECK(std::errc::file_exists == ec);
				}

				SECTION("with option skip_existing") {
					src->copy("foo/dog", *dst, "dog", fs::copy_options::skip_existing);

					std::string content;
					std::getline(*dst->open_read("dog"), content);
					CHECK("howl" == content);
				}

				SECTION("with option overwrite") {
					src->copy("foo/dog", *dst, "dog", fs::copy_options::overwrite_existing);

					std::string content;
					std::getline(*dst->open_read("dog"), content);
					CHECK("woof" == content);
				}

				SECTION("with option update_existing") {
					src->last_write_time("foo/dog", fs::file_time_type(std::chrono::seconds(10)));
					dst->last_write_time("dog", fs::file_time_type(std::chrono::seconds(20)));
					src->copy("foo/dog", *dst, "dog", fs::copy_options::update_existing);

					std::string content;
					std::getline(*dst->open_read("dog"), content);
					CHECK("howl" == content);

					src->last_write_time("foo/dog", fs::file_time_type(std::chrono::seconds(30)));
					src->copy("foo/dog", *dst, "dog", fs::copy_options::update_existing);

					std::getline(*dst->open_read("dog"), content);
					CHECK("woof" == content);
				}
			}
		}

		SECTION("directory") {
			SECTION("without option") {
				src->copy("foo", *dst, "foo");
				REQUIRE(dst->is_directory("foo"));
				CHECK(not dst->is_directory("foo/bar"));
				CHECK(not dst->is_directory("foo/bar/baz"));
				CHECK(not dst->is_directory("foo/qux"));
				CHECK(dst->is_regular_file("foo/dog"));
				CHECK(not dst->is_symlink("foo/cat"));
			}

			SECTION("with option copy_symlinks") {
				src->copy("foo", *dst, "foo", fs::copy_options::copy_symlinks);
				CHECK(not dst->is_directory("foo"));
			}

			SECTION("with option directories_only") {
				src->copy("foo", *dst, "foo", fs::copy_options::directories_only);
				CHECK(not dst->is_directory("foo"));
			}

			SECTION("with option recursive") {
				src->copy("foo", *dst, "foo", fs::copy_options::recursive);
				REQUIRE(dst->is_directory("foo"));
				CHECK(dst->is_directory("foo/bar"));
				CHECK(dst->is_directory("foo/bar/baz"));
				CHECK(dst->is_directory("foo/qux"));
				CHECK(dst->is_regular_file("foo/dog"));
				CHECK(not dst->is_symlink("foo/cat"));
			}

			SECTION("with option recursive and") {
				auto opts = fs::copy_options::recursive;

				SECTION("copy_symlinks") {
					opts |= fs::copy_options::copy_symlinks;
					src->copy("foo", *dst, "foo", opts);
					REQUIRE(dst->is_directory("foo"));
					CHECK(dst->is_directory("foo/bar"));
					CHECK(dst->is_directory("foo/bar/baz"));
					CHECK(dst->is_directory("foo/qux"));
					CHECK(dst->is_regular_file("foo/dog"));
					CHECK(dst->is_symlink("foo/cat"));
				}

				SECTION("directories_only") {
					opts |= fs::copy_options::directories_only;
					src->copy("foo", *dst, "foo", opts);
					REQUIRE(dst->is_directory("foo"));
					CHECK(dst->is_directory("foo/bar"));
					CHECK(dst->is_directory("foo/bar/baz"));
					CHECK(dst->is_directory("foo/qux"));
					CHECK(not dst->is_regular_file("foo/dog"));
					CHECK(not dst->is_symlink("foo/cat"));
				}

				SECTION("directories_only to existing directory") {
					opts |= fs::copy_options::directories_only;
					dst->create_directories("foo/bar");

					src->copy("foo", *dst, "foo", opts);
					REQUIRE(dst->is_directory("foo"));
					CHECK(dst->is_directory("foo/bar"));
					CHECK(dst->is_directory("foo/bar/baz"));
					CHECK(dst->is_directory("foo/qux"));
					CHECK(not dst->is_regular_file("foo/dog"));
					CHECK(not dst->is_symlink("foo/cat"));
				}
			}

			SECTION("to existing regular file") {
				*dst->open_write("dog") << "howl";
				REQUIRE(dst->is_regular_file("dog"));

				std::error_code ec;
				src->copy("foo", *dst, "dog", ec);
				CHECK(std::errc::is_a_directory == ec);
			}
		}
	}
};

class CopyFromOsFsToOsFs: public TestCopyFixture {
   public:
	std::shared_ptr<vfs::Fs> make_src_fs() override {
		return vfs::make_os_fs();
	}

	std::shared_ptr<vfs::Fs> make_dst_fs() override {
		return vfs::make_os_fs();
	}
};

METHOD_AS_TEST_CASE(TestCopy<CopyFromOsFsToOsFs>::test, "Copy from OsFs to OsFs");

class CopyFromOsFsToVfs: public TestCopyFixture {
   public:
	std::shared_ptr<vfs::Fs> make_src_fs() override {
		return vfs::make_os_fs();
	}

	std::shared_ptr<vfs::Fs> make_dst_fs() override {
		return vfs::make_vfs();
	}
};

METHOD_AS_TEST_CASE(TestCopy<CopyFromOsFsToVfs>::test, "Copy from OsFs to Vfs");

class CopyFromVfsToOsFs: public TestCopyFixture {
   public:
	std::shared_ptr<vfs::Fs> make_src_fs() override {
		return vfs::make_vfs();
	}

	std::shared_ptr<vfs::Fs> make_dst_fs() override {
		return vfs::make_os_fs();
	}
};

METHOD_AS_TEST_CASE(TestCopy<CopyFromOsFsToVfs>::test, "Copy from Vfs to OsFs");

class CopyFromVfsToVfs: public TestCopyFixture {
   public:
	std::shared_ptr<vfs::Fs> make_src_fs() override {
		return vfs::make_vfs();
	}

	std::shared_ptr<vfs::Fs> make_dst_fs() override {
		return vfs::make_vfs();
	}
};

METHOD_AS_TEST_CASE(TestCopy<CopyFromVfsToVfs>::test, "Copy from Vfs to Vfs");
