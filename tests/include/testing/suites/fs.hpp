#include <concepts>
#include <memory>
#include <string>
#include <unordered_set>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/directory_iterator.hpp>
#include <vfs/fs.hpp>

#include <vfs/impl/utils.hpp>

namespace testing {

class TestFsFixture {
   public:
	virtual std::shared_ptr<vfs::Fs> make() = 0;
};

template<std::derived_from<TestFsFixture> Fixture>
class TestFsBasic {
   public:
	void test() {
		using namespace std::filesystem;

		Fixture fixture;

		std::shared_ptr<vfs::Fs> fs = fixture.make();

		auto const sandbox = fs->temp_directory_path() / "vfs-test" / vfs::impl::random_string(32, vfs::impl::Alphanumeric);
		INFO("sandbox: " + sandbox.string());

		fs->create_directories(sandbox);
		fs = fs->current_path(sandbox);
		REQUIRE(sandbox.is_absolute());
		REQUIRE(sandbox == fs->current_path());

		SECTION("::open_*") {
			SECTION("reading a file that does not exist fails") {
				auto const f = fs->open_read("foo");
				CHECK(not f->good());
				CHECK(f->fail());
				CHECK(not f->bad());
				CHECK(not f->eof());
			}

			SECTION("writing a file creates a new file if the file does not exist") {
				REQUIRE(not fs->exists("foo"));

				auto f = fs->open_write("foo");
				CHECK(fs->exists("foo"));
				CHECK(f->good());
				CHECK(not f->fail());
				CHECK(not f->bad());
				CHECK(not f->eof());
			}

			SECTION("read write") {
				*fs->open_write("foo") << "Lorem ipsum";

				std::string content;
				std::getline(*fs->open_read("foo"), content);
				CHECK("Lorem ipsum" == content);
			}
		}

		SECTION("::canonical") {
			// /
			// + /foo *
			//   + /bar
			// + baz -> foo/bar
			// + qux -> baz
			// + dog
			// + cat -> dog
			// + void -> not-exists
			fs->create_directories("foo/bar");
			fs->create_symlink("foo/bar", "baz");
			fs->create_symlink("baz", "qux");
			fs->open_write("dog");
			fs->create_symlink("dog", "cat");
			fs = fs->current_path("foo");

			SECTION("valid paths") {
				CHECK("/" == fs->canonical("/"));
				CHECK("/" == fs->canonical("/.."));           // Parent of root is root.
				CHECK(sandbox / "foo" == fs->canonical(""));  // Empty resolved to current directory.
				CHECK(sandbox / "foo" == fs->canonical("."));
				CHECK(sandbox / "foo/bar" == fs->canonical("bar"));
				CHECK(sandbox / "foo/bar" == fs->canonical("./bar"));
				CHECK(sandbox / "foo/bar" == fs->canonical("././bar"));
				CHECK(sandbox / "foo/bar" == fs->canonical("../baz"));
				CHECK(sandbox / "foo/bar" == fs->canonical("../baz/../../baz"));
				CHECK(sandbox / "foo/bar" == fs->canonical("../qux"));  // Symlink chain is followed.
				CHECK(sandbox / "foo" == fs->canonical("../baz/.."));   // No lexical normalize.
				CHECK(sandbox / "dog" == fs->canonical("../cat"));
			}

			SECTION("weakly") {
				CHECK("/" == fs->weakly_canonical("/"));
				CHECK("/" == fs->weakly_canonical("/.."));  // Parent of root is root.
				CHECK("" == fs->weakly_canonical(""));      // Empty resolved to empty.
				CHECK(sandbox / "foo" == fs->weakly_canonical("."));
				CHECK(sandbox / "foo/bar" == fs->weakly_canonical("bar"));
				CHECK(sandbox / "foo/bar" == fs->weakly_canonical("./bar"));
				CHECK(sandbox / "foo/bar" == fs->weakly_canonical("././bar"));
				CHECK(sandbox / "foo/bar" == fs->weakly_canonical("../baz"));
				CHECK(sandbox / "foo/bar" == fs->weakly_canonical("../baz/../../baz"));
				CHECK(sandbox / "foo/bar" == fs->weakly_canonical("../qux"));  // Symlink chain is followed.
				CHECK(sandbox / "foo" == fs->weakly_canonical("../baz/.."));   // No lexical normalize.
				CHECK(sandbox / "dog" == fs->weakly_canonical("../cat"));

				CHECK(sandbox / "foo/" == fs->weakly_canonical("bar/baz/../.."));                // Resulting path is in normal form.
				CHECK(sandbox / "not-exists" == fs->weakly_canonical("../not-exists"));
				CHECK(sandbox / "void/somewhere" == fs->weakly_canonical("../void/somewhere"));  // Symlink path is resolved if the target is not exists.

				// Resulting lexical normal of given path if the first entry does not exist.
				CHECK("not-exists" == fs->weakly_canonical("not-exists"));
				CHECK("." == fs->weakly_canonical("not-exists/.."));
			}

			SECTION("file is accessed as a directory if there is trailing slash") {
				std::error_code ec;
				fs->canonical(sandbox / "dog/", ec);
				CHECK(std::errc::not_a_directory == ec);
			}

			SECTION("file must exist") {
				std::error_code ec;
				fs->canonical("not_exists", ec);
				CHECK(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("target of symlink must exist") {
				std::error_code ec;
				fs->canonical(sandbox / "void", ec);
				CHECK(std::errc::no_such_file_or_directory == ec);
			}
		}

		SECTION("::copy") {
			// /
			// + /foo
			//   + /bar
			//     + /baz
			//   + dog
			//   + cat -> ./dog
			fs->create_directories("foo/bar/baz");
			*fs->open_write("foo/dog") << "woof";
			fs->create_symlink("./dog", "foo/cat");

			SECTION("recursive") {
				fs->copy("foo", "foo_", copy_options::recursive | copy_options::copy_symlinks);

				CHECK(fs->is_directory("foo_/bar/baz"));
				CHECK(fs->is_regular_file("foo_/dog"));
				CHECK(fs->is_symlink("foo_/cat"));
			}

			SECTION("directory only") {
				// TODO: symlink should be skipped or not according to its target.
				fs->copy("foo", "foo_", copy_options::recursive | copy_options::directories_only | copy_options::skip_symlinks);

				CHECK(fs->is_directory("foo_/bar/baz"));
				CHECK(not fs->exists("foo_/dog"));
				CHECK(not fs->exists("foo_/cat"));
			}

			// TODO: more tests
		}

		SECTION("::copy_file") {
			*fs->open_write("foo") << "Lorem ipsum";
			REQUIRE(fs->is_regular_file("foo"));

			SECTION("source does not exist") {
				REQUIRE(not fs->exists("not-exists"));

				std::error_code ec;
				CHECK(not fs->copy_file("not-exists", "bar", ec));
				CHECK(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("source is not a regular file") {
				fs->create_directory("qux");
				REQUIRE(fs->exists("qux"));
				REQUIRE(not fs->is_regular_file("qux"));

				std::error_code ec;
				CHECK(not fs->copy_file("qux", "bar", ec));
				CHECK(std::errc::invalid_argument == ec);
			}

			SECTION("create a file if the destination does not exist") {
				REQUIRE(not fs->exists("bar"));

				CHECK(fs->copy_file("foo", "bar"));

				std::string content;
				std::getline(*fs->open_read("bar"), content);
				CHECK("Lorem ipsum" == content);
			}

			SECTION("to same file") {
				std::error_code ec;
				CHECK(not fs->copy_file("foo", "foo", ec));
				CHECK(std::errc::file_exists == ec);
			}

			SECTION("to non-regular file") {
				fs->create_directory("bar");
				REQUIRE(fs->exists("bar"));
				REQUIRE(not fs->is_regular_file("bar"));

				std::error_code ec;
				CHECK(not fs->copy_file("foo", "bar", ec));
				CHECK(std::errc::invalid_argument == ec);
			}

			SECTION("to existing regular file") {
				*fs->open_write("bar") << "dolor sit amet";
				REQUIRE(fs->is_regular_file("bar"));

				SECTION("with skip") {
					fs->copy_file("foo", "bar", copy_options::skip_existing);

					std::string content;
					std::getline(*fs->open_read("bar"), content);
					CHECK("dolor sit amet" == content);
				}

				SECTION("with overwrite") {
					fs->copy_file("foo", "bar", copy_options::overwrite_existing);

					std::string content;
					std::getline(*fs->open_read("bar"), content);
					CHECK("Lorem ipsum" == content);
				}

				SECTION("with update") {
					// TODO:
				}
			}
		}

		SECTION("::create_directory") {
			SECTION("in an existing directory") {
				CHECK(fs->create_directory("foo"));
				CHECK(fs->is_directory("foo"));
			}

			SECTION("to an existing directory") {
				fs->create_directory("foo");
				REQUIRE(fs->is_directory("foo"));

				// Returns `false` since no new directory is created at the time.
				CHECK(not fs->create_directory("foo"));
			}

			SECTION("in a directory that does not exist") {
				REQUIRE(not fs->exists("foo"));

				std::error_code ec;
				CHECK(not fs->create_directory("foo/bar", ec));
				CHECK(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("in a location where the file is not a directory") {
				*fs->open_write("foo");
				REQUIRE(fs->exists("foo"));
				REQUIRE(!fs->is_directory("foo"));

				std::error_code ec;
				CHECK(not fs->create_directory("foo/bar", ec));
				CHECK(std::errc::not_a_directory == ec);
			}
		}

		SECTION("::create_directories") {
			SECTION("in an existing directory") {
				CHECK(fs->create_directories("foo/bar"));
				CHECK(fs->is_directory("foo/bar"));
			}

			SECTION("to an existing directory") {
				fs->create_directories("foo/bar");
				REQUIRE(fs->is_directory("foo/bar"));

				// Returns `false` since no new directory is created at the time.
				CHECK(not fs->create_directories("foo/bar"));
			}

			SECTION("in a directory that does not exist") {
				REQUIRE(not fs->exists("foo"));

				std::error_code ec;
				CHECK(fs->create_directories("foo/bar", ec));
			}

			SECTION("in a location where the file is not a directory") {
				*fs->open_write("foo");
				REQUIRE(fs->exists("foo"));
				REQUIRE(!fs->is_directory("foo"));

				std::error_code ec;
				CHECK(not fs->create_directories("foo/bar", ec));
				CHECK(std::errc::not_a_directory == ec);
			}
		}

		SECTION("::create_hard_link") {
			SECTION("to a regular file") {
				*fs->open_write("foo") << "Lorem ipsum";
				REQUIRE(fs->is_regular_file("foo"));

				fs->create_hard_link("foo", "bar");
				CHECK(fs->is_regular_file("bar"));

				std::string content;
				std::getline(*fs->open_read("bar"), content);
				CHECK("Lorem ipsum" == content);

				fs->create_hard_link("bar", "baz");
				CHECK(fs->is_regular_file("baz"));

				content.clear();
				std::getline(*fs->open_read("baz"), content);
				CHECK("Lorem ipsum" == content);

				fs->remove("foo");
				REQUIRE(not fs->exists("foo"));

				content.clear();
				std::getline(*fs->open_read("bar"), content);
				CHECK("Lorem ipsum" == content);
			}

			// TODO: hard link to directory?
		}

		SECTION("::create_symlink") {
			fs->create_directory("foo");
			REQUIRE(fs->exists("foo"));

			SECTION("in an existing directory") {
				fs->create_symlink("foo", "bar");

				CHECK("foo" == fs->read_symlink("bar"));
			}

			SECTION("in a directory that does not exist") {
				REQUIRE(not fs->exists("bar"));

				std::error_code ec;
				fs->create_symlink("foo", "bar/baz", ec);
				CHECK(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("to file that does not exist") {
				fs->create_symlink("baz", "bar");

				CHECK("baz" == fs->read_symlink("bar"));
			}
		}

		SECTION("::current_path") {
			SECTION("change to existing directory") {
				fs->create_directory("foo");
				REQUIRE(fs->is_directory("foo"));

				auto const foo = fs->current_path("foo");
				CHECK(sandbox / "foo" == foo->current_path());
			}

			SECTION("change to existing directory with an r-value of Fs") {
				fs->create_directories("foo/bar");
				REQUIRE(fs->is_directory("foo"));

				auto const bar = fs->current_path("foo")->current_path("./bar");
				CHECK(sandbox / "foo/bar" == bar->current_path());
			}

			SECTION("change to a path that does not exist") {
				REQUIRE(not fs->exists("foo"));

				std::error_code ec;
				CHECK(nullptr == fs->current_path("foo", ec));
				CHECK(std::errc::no_such_file_or_directory == ec);
			}

			SECTION("change to a path that is not a directory") {
				fs->open_write("foo");

				std::error_code ec;
				CHECK(nullptr == fs->current_path("foo", ec));
				CHECK(std::errc::not_a_directory == ec);
			}
		}

		SECTION("::equivalent") {
			fs->create_directory("foo");
			REQUIRE(fs->exists("foo"));

			SECTION("true if two path is resolved to same file") {
				CHECK(fs->equivalent("/", "/"));
				CHECK(fs->equivalent("/", "/.."));
				CHECK(fs->equivalent("/..", "/"));
				CHECK(fs->equivalent(".", sandbox));
				CHECK(fs->equivalent(sandbox, "."));
				CHECK(fs->equivalent("foo", "foo"));
			}

			SECTION("false if two path is resolved to different file") {
				CHECK(not fs->equivalent("/", "foo"));
				CHECK(not fs->equivalent("foo", "/"));
			}

			SECTION("symlink is followed") {
				fs->create_symlink("foo", "bar");
				REQUIRE("foo" == fs->read_symlink("bar"));

				CHECK(fs->equivalent("foo", "bar"));
			}

			SECTION("one of given path does not exist") {
				REQUIRE(not fs->exists("bar"));

				std::error_code ec;
				CHECK(not fs->equivalent("bar", "foo", ec));
				CHECK(0 == ec.value());

				ec.clear();
				CHECK(not fs->equivalent("foo", "bar", ec));
				CHECK(0 == ec.value());
			}

			SECTION("both given path does not exist") {
				REQUIRE(not fs->exists("bar"));
				REQUIRE(not fs->exists("baz"));

				std::error_code ec;
				CHECK(not fs->equivalent("bar", "baz", ec));
				CHECK(std::errc::no_such_file_or_directory == ec);

				ec.clear();
				CHECK(not fs->equivalent("baz", "bar", ec));
				CHECK(std::errc::no_such_file_or_directory == ec);
			}
		}

		SECTION("hard_link_count") {
			*fs->open_write("foo");
			REQUIRE(fs->is_regular_file("foo"));

			CHECK(1 == fs->hard_link_count("foo"));

			fs->create_hard_link("foo", "bar");
			CHECK(2 == fs->hard_link_count("foo"));
			CHECK(2 == fs->hard_link_count("bar"));

			fs->create_hard_link("foo", "baz");
			CHECK(3 == fs->hard_link_count("foo"));
			CHECK(3 == fs->hard_link_count("bar"));
			CHECK(3 == fs->hard_link_count("baz"));

			fs->remove("foo");
			REQUIRE(not fs->exists("foo"));
			CHECK(2 == fs->hard_link_count("bar"));
			CHECK(2 == fs->hard_link_count("baz"));
		}

		SECTION("::read_symlink") {
			SECTION("target may not necessarily exist") {
				fs->create_symlink("not-exists", "bar");
				REQUIRE(fs->is_symlink("bar"));

				CHECK("not-exists" == fs->read_symlink("bar"));
			}

			SECTION("of file that is not a symbolic link") {
				fs->create_directory("foo");
				REQUIRE(fs->exists("foo"));

				std::error_code ec;
				fs->read_symlink("foo", ec);
				CHECK(std::errc::invalid_argument == ec);
			}
		}

		SECTION("::remove") {
			SECTION("directory that does not exist") {
				REQUIRE(not fs->exists("foo"));
				CHECK(not fs->remove("foo"));
			}

			SECTION("directory that is empty") {
				fs->create_directory("foo");
				REQUIRE(fs->is_directory("foo"));

				CHECK(fs->remove("foo"));
			}

			SECTION("directory that is not empty") {
				fs->create_directories("foo/bar");
				REQUIRE(not fs->is_empty("foo"));

				std::error_code ec;
				CHECK(not fs->remove("foo", ec));
				CHECK(std::errc::directory_not_empty == ec);
			}

			SECTION("symlink") {
				fs->create_directory("foo");
				REQUIRE(fs->exists("foo"));

				fs->create_symlink("foo", "bar");
				REQUIRE(fs->is_symlink("bar"));

				CHECK(fs->remove("bar"));
				CHECK(fs->exists("foo"));
			}
		}

		SECTION("::rename") {
			SECTION("from file") {
				*fs->open_write("foo") << "Lorem ipsum";
				REQUIRE(fs->is_regular_file("foo"));

				SECTION("to same file") {
					fs->rename("foo", "foo");
					CHECK(fs->is_regular_file("foo"));
				}

				SECTION("to same file that is hard linked") {
					fs->create_hard_link("foo", "bar");
					REQUIRE(fs->equivalent("foo", "bar"));

					fs->rename("foo", "foo");
					CHECK(fs->is_regular_file("foo"));
					CHECK(fs->is_regular_file("bar"));
					CHECK(fs->equivalent("foo", "bar"));
				}

				SECTION("to file") {
					*fs->open_write("bar") << "dolor sit amet";
					REQUIRE(fs->is_regular_file("bar"));

					fs->rename("foo", "bar");
					CHECK(not fs->exists("foo"));
					CHECK(fs->is_regular_file("bar"));

					std::string content;
					std::getline(*fs->open_read("bar"), content);
					CHECK("Lorem ipsum" == content);
				}

				SECTION("to directory") {
					fs->create_directory("bar");
					REQUIRE(fs->is_directory("bar"));

					std::error_code ec;
					fs->rename("foo", "bar", ec);
					CHECK(std::errc::is_a_directory == ec);
					CHECK(fs->is_regular_file("foo"));
				}
			}

			SECTION("from directory") {
				fs->create_directories("foo/qux");
				REQUIRE(fs->is_directory("foo/qux"));

				SECTION("to same directory") {
					fs->rename("foo", "foo");
					REQUIRE(fs->is_directory("foo"));
				}

				SECTION("to directory that is empty") {
					fs->create_directory("bar");
					REQUIRE(fs->is_directory("bar"));

					fs->rename("foo", "bar");
					CHECK(not fs->exists("foo"));
					CHECK(fs->is_directory("bar/qux"));
				}

				SECTION("to directory that is not empty") {
					fs->create_directories("bar/baz");
					REQUIRE(fs->is_directory("bar/baz"));

					std::error_code ec;
					fs->rename("foo", "bar", ec);
					CHECK(std::errc::directory_not_empty == ec);
					CHECK(fs->is_directory("foo/qux"));
				}

				SECTION("to file") {
					*fs->open_write("bar") << "Lorem ipsum";
					REQUIRE(fs->is_regular_file("bar"));

					std::error_code ec;
					fs->rename("foo", "bar", ec);
					CHECK(std::errc::not_a_directory == ec);
					CHECK(fs->is_directory("foo/qux"));
				}

				SECTION("to its children") {
					std::error_code ec;
					fs->rename("foo", "foo/bar", ec);
					CHECK(std::errc::invalid_argument == ec);
					CHECK(fs->is_directory("foo/qux"));
					CHECK(not fs->exists("foo/bar"));
				}
			}
		}

		SECTION("::iterate_directory") {
			fs->open_write("foo");
			fs->open_write("bar");

			std::unordered_set<std::string> filenames;

			for(auto const entry: fs->iterate_directory()) {
				CHECK(entry.is_regular_file());
				filenames.insert(entry.path().filename());
			}

			CHECK(std::unordered_set<std::string>{"foo", "bar"} == filenames);
		}

		fs->remove_all(sandbox);
	}
};

}  // namespace testing
