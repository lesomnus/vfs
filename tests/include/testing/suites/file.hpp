#include <concepts>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/file.hpp>
#include <vfs/impl/storage.hpp>

namespace fs = std::filesystem;

namespace testing {

class TestFileFixture {
   public:
	virtual std::shared_ptr<vfs::impl::Storage> make() = 0;
};

template<std::derived_from<TestFileFixture> Fixture>
class TestFile {
   public:
	void test() {
		Fixture fixture;

		std::shared_ptr<vfs::impl::Storage> const storage = fixture.make();

		SECTION("File") {
			SECTION("::operator==()") {
				auto d1 = storage->make_directory();
				CHECK(*d1 == *d1);

				auto d2 = storage->make_directory();
				CHECK(*d1 != *d2);

				d1->insert("foo", d2);
				auto foo = d1->next("foo");
				CHECK(nullptr != foo);
				CHECK(*d2 == *foo);
			}
		}

		SECTION("RegularFile") {
			SECTION("::size") {
				auto f = storage->make_regular_file();
				CHECK(0 == f->size());

				constexpr char quote[] = "Lorem ipsum";
				*f->open_write() << quote;
				CHECK(sizeof(quote) - 1 == f->size());
			}

			SECTION("::last_write_time") {
				auto f = storage->make_regular_file();

				auto const t0 = f->last_write_time();
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
				*f->open_write() << "Lorem ipsum";
				auto const t1 = f->last_write_time();
				CHECK(std::chrono::milliseconds(25) <= (t1 - t0));
				CHECK(std::chrono::milliseconds(35) >= (t1 - t0));

				f->last_write_time(t0);
				CHECK(t0 == f->last_write_time());
			}
		}

		SECTION("Directory") {
			SECTION("insert a file into the directory") {
				auto root = storage->make_directory();
				{
					auto f = storage->make_regular_file();
					*f->open_write() << "Lorem ipsum";

					CHECK(root->insert("foo", f));
				}

				auto foo_f = root->next("foo");
				REQUIRE(nullptr != foo_f);

				auto foo_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(foo_f);
				REQUIRE(nullptr != foo_r);

				std::string line;
				std::getline(*foo_r->open_read(), line);
				CHECK("Lorem ipsum" == line);
			}

			SECTION("insert a directory into the directory") {
				auto root = storage->make_directory();
				{
					auto d = storage->make_directory();
					auto f = storage->make_regular_file();
					*f->open_write() << "Lorem ipsum";

					CHECK(root->insert("foo", d));
					CHECK(d->insert("bar", f));
				}

				auto foo_f = root->next("foo");
				REQUIRE(nullptr != foo_f);

				auto foo_d = std::dynamic_pointer_cast<vfs::impl::Directory>(foo_f);
				REQUIRE(nullptr != foo_d);

				auto bar_f = foo_d->next("bar");
				REQUIRE(nullptr != foo_f);

				auto bar_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(bar_f);
				REQUIRE(nullptr != bar_r);

				std::string line;
				std::getline(*bar_r->open_read(), line);
				CHECK("Lorem ipsum" == line);
			}

			SECTION("iterate a directory") {
				auto root = storage->make_directory();
				CHECK(root->empty());
				CHECK(root->end() == root->begin());

				root->insert("foo", storage->make_regular_file());
				root->insert("bar", storage->make_directory());
				CHECK(not root->empty());
				CHECK(root->end() != root->begin());

				std::unordered_map<std::string, std::shared_ptr<vfs::impl::File>> files;
				for(auto const& [name, file]: *root) {
					files.insert(std::make_pair(name, file));
				}

				CHECK(2 == files.size());
				REQUIRE(files.contains("foo"));
				REQUIRE(files.contains("bar"));

				CHECK(nullptr != std::dynamic_pointer_cast<vfs::impl::RegularFile>(files.at("foo")));
				CHECK(nullptr != std::dynamic_pointer_cast<vfs::impl::Directory>(files.at("bar")));
			}
		}
	}
};

}  // namespace testing
