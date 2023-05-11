#include <concepts>
#include <memory>
#include <string>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

namespace testing {

class TestFsFixture {
   public:
	virtual std::shared_ptr<vfs::Fs> make() = 0;
};

// TODO: current fail test will be failed since different fs reports different error code for the same error reason.

template<std::derived_from<TestFsFixture> Fixture>
class TestFsBasic {
   public:
	void test() {
		using namespace std::filesystem;

		Fixture fixture;

		std::shared_ptr<vfs::Fs> fs = fixture.make();

		auto cwd = fs->temp_directory_path() / "vfs-test";
		fs->create_directory(cwd);
		fs = fs->current_path(cwd);
		REQUIRE(cwd.is_absolute());
		REQUIRE(cwd == fs->current_path());

		SECTION("::open_*") {
			SECTION("reading a file that does not exist fails") {
				auto const f = fs->open_read("./foo");
				REQUIRE(not f->good());
				REQUIRE(f->fail());
				REQUIRE(not f->bad());
				REQUIRE(not f->eof());
			}

			SECTION("writing a file creates a new file if the file does not exist") {
				REQUIRE(not fs->exists("./foo"));

				auto f = fs->open_write("./foo");
				REQUIRE(fs->exists("./foo"));
				REQUIRE(f->good());
				REQUIRE(not f->fail());
				REQUIRE(not f->bad());
				REQUIRE(not f->eof());
			}

			SECTION("read write") {
				*fs->open_write("./foo") << "Lorem ipsum";

				std::string content;
				std::getline(*fs->open_read("./foo"), content);
				REQUIRE("Lorem ipsum" == content);
			}
		}

		SECTION("::canonical") {
		}

		SECTION("::weakly_canonical") {
			REQUIRE((cwd / "foo") == fs->weakly_canonical("./foo"));
		}

		SECTION("::copy") {
			// /
			// + /foo
			//   + /bar
			//     + /baz
			//   + dog
			//   + cat -> ./dog
			fs->create_directories("./foo/bar/baz");
			*fs->open_write("./foo/dog") << "woof";
			fs->create_symlink("./dog", "./foo/cat");

			SECTION("recursive") {
				fs->copy("./foo", "./foo_", copy_options::recursive | copy_options::copy_symlinks);

				REQUIRE(fs->is_directory("./foo_/bar/baz"));
				REQUIRE(fs->is_regular_file("./foo_/dog"));
				REQUIRE(fs->is_symlink("./foo_/cat"));
			}

			SECTION("directory only") {
				// TODO: symlink should be skipped or not according to its target.
				fs->copy("./foo", "./foo_", copy_options::recursive | copy_options::directories_only | copy_options::skip_symlinks);

				REQUIRE(fs->is_directory("./foo_/bar/baz"));
				REQUIRE(not fs->exists("./foo_/dog"));
				REQUIRE(not fs->exists("./foo_/cat"));
			}

			// TODO: more tests
		}

		SECTION("::copy_file") {
			*fs->open_write("./foo") << "Lorem ipsum";

			SECTION("create a file if the destination does not exist") {
				REQUIRE(fs->copy_file("./foo", "./bar"));

				std::string content;
				std::getline(*fs->open_read("./bar"), content);
				REQUIRE("Lorem ipsum" == content);
			}

			SECTION("to same file") {
				std::error_code ec;
				REQUIRE(not fs->copy_file("./foo", "./foo", ec));
				REQUIRE(std::errc::file_exists == ec);
			}

			SECTION("to non-regular file") {
				REQUIRE(fs->create_directory("./bar"));

				std::error_code ec;
				REQUIRE(not fs->copy_file("./foo", "./bar", ec));
				REQUIRE(std::errc::invalid_argument == ec);
			}

			SECTION("to existing file") {
				*fs->open_write("./bar") << "dolor sit amet";

				SECTION("with skip") {
					fs->copy_file("./foo", "./bar", copy_options::skip_existing);

					std::string content;
					std::getline(*fs->open_read("./bar"), content);
					REQUIRE("dolor sit amet" == content);
				}

				SECTION("with overwrite") {
					fs->copy_file("./foo", "./bar", copy_options::overwrite_existing);

					std::string content;
					std::getline(*fs->open_read("./bar"), content);
					REQUIRE("Lorem ipsum" == content);
				}

				SECTION("with update") {
					// TODO:
				}
			}
		}

		SECTION("::create_directory") {
			SECTION("in an existing directory") {
				REQUIRE(fs->create_directory("foo"));

				file_status const status = fs->status(cwd / "foo");
				REQUIRE(file_type::directory == status.type());
			}

			SECTION("in a directory that does not exist") {
				REQUIRE(not fs->exists("./bar"));

				std::error_code ec;
				REQUIRE(not fs->create_directory("./bar/baz", ec));
				REQUIRE(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("in a location where the file is not a directory") {
				// TODO
			}
		}

		SECTION("::create_directories") {
			SECTION("in a directory that does not exist") {
				REQUIRE(not fs->exists("./foo"));

				REQUIRE(fs->create_directories("./foo/bar"));

				file_status const status = fs->status("./foo/bar");
				REQUIRE(file_type::directory == status.type());
			}

			SECTION("in an existing directory") {
				REQUIRE(fs->create_directory("./foo"));

				REQUIRE(fs->create_directories("./foo/bar/baz"));

				file_status const status = fs->status("./foo/bar/baz");
				REQUIRE(file_type::directory == status.type());
			}
		}

		SECTION("::current_path") {
			SECTION("change to existing directory") {
				fs->create_directory("./foo");

				auto const foo = fs->current_path("./foo");
				REQUIRE((cwd / "foo") == foo->current_path());
			}

			SECTION("change to existing directory with r-value of Fs") {
				fs->create_directories("./foo/bar");

				auto const bar = fs->current_path("./foo")->current_path("./bar");
				REQUIRE((cwd / "foo/bar") == bar->current_path());
			}
		}

		SECTION("::create_symlink") {
			REQUIRE(fs->create_directory("./foo"));

			SECTION("in an existing directory") {
				fs->create_symlink("./foo", "./bar");

				file_status status = fs->symlink_status("./bar");
				REQUIRE(file_type::symlink == status.type());

				status = fs->status("./bar");
				REQUIRE(file_type::directory == status.type());
				REQUIRE(fs->create_directory("./bar/baz"));
			}

			SECTION("in a directory that does not exist") {
				REQUIRE(not fs->exists("./bar"));

				std::error_code ec;
				fs->create_symlink("./foo", "./bar/baz", ec);
				REQUIRE(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("to file that does not exist") {
				fs->create_symlink("./baz", "./bar");

				file_status status = fs->symlink_status(cwd / "bar");
				REQUIRE(file_type::symlink == status.type());

				status = fs->status(cwd / "bar");
				REQUIRE(file_type::not_found == status.type());
			}
		}

		SECTION("::equivalent") {
			fs->create_directory("./foo");

			SECTION("true if two path is resolved to same file") {
				REQUIRE(fs->equivalent("/", "/"));
				REQUIRE(fs->equivalent("/", "/.."));
				REQUIRE(fs->equivalent("/..", "/"));
				REQUIRE(fs->equivalent(".", cwd));
				REQUIRE(fs->equivalent(cwd, "."));
				REQUIRE(fs->equivalent("./foo", "./foo"));
			}

			SECTION("false if two path is resolved to different file") {
				REQUIRE(not fs->equivalent("/", "./foo"));
				REQUIRE(not fs->equivalent("./foo", "/"));
			}

			SECTION("symlink is followed") {
				fs->create_symlink("./foo", "./bar");

				REQUIRE(fs->equivalent("./foo", "./bar"));
			}
		}

		SECTION("::read_symlink") {
			fs->create_directory("./foo");

			SECTION("target may not necessarily exist") {
				fs->create_symlink("./not-exists", "./bar");

				REQUIRE("./not-exists" == fs->read_symlink("./bar"));
			}

			SECTION("that is not a symbolic link") {
				std::error_code ec;
				fs->read_symlink("./foo", ec);
				REQUIRE(std::errc::invalid_argument == ec);
			}
		}

		SECTION("::remove") {
			SECTION("directory that does not exist") {
				REQUIRE(not fs->exists("./foo"));
				REQUIRE(not fs->remove("./foo"));
			}

			SECTION("empty directory") {
				fs->create_directory("./foo");

				REQUIRE(fs->remove("./foo"));
			}

			SECTION("directory that is not empty") {
				fs->create_directories("./foo/bar");

				std::error_code ec;
				REQUIRE(not fs->remove("./foo", ec));
				REQUIRE(std::errc::directory_not_empty == ec);
			}

			SECTION("symlink") {
				fs->create_symlink("./not-exists", "./foo");

				REQUIRE(fs->remove("./foo"));
			}
		}
	}
};

}  // namespace testing
