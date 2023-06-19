#include <concepts>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/file.hpp>

namespace fs = std::filesystem;

namespace testing {
namespace suites {

class TestFileFixture {
   public:
	virtual std::shared_ptr<vfs::impl::Directory> make() = 0;
};

template<std::derived_from<TestFileFixture> Fixture>
class TestFile {
   public:
	void test() {
		Fixture fixture;

		std::shared_ptr<vfs::impl::Directory> const sandbox = fixture.make();

		SECTION("File") {
			SECTION("::operator==()") {
				auto [foo, ok1] = sandbox->emplace_directory("foo");
				REQUIRE(ok1);
				CHECK(*foo == *foo);

				auto [bar, ok2] = sandbox->emplace_directory("bar");
				REQUIRE(ok2);
				CHECK(*foo != *bar);

				foo->insert("baz", bar);
				auto baz = foo->next("baz");
				CHECK(nullptr != baz);
			}
		}

		SECTION("RegularFile") {
			SECTION("::size") {
				auto [f, ok] = sandbox->emplace_regular_file("foo");
				REQUIRE(ok);
				CHECK(0 == f->size());

				constexpr char quote[] = "Lorem ipsum";
				*f->open_write() << quote;
				CHECK(sizeof(quote) - 1 == f->size());
			}

			SECTION("::last_write_time") {
				auto [f, ok] = sandbox->emplace_regular_file("foo");
				REQUIRE(ok);

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
				auto [foo, ok] = sandbox->emplace_directory("foo");
				REQUIRE(ok);

				{
					auto [bar, ok] = sandbox->emplace_regular_file("bar");
					REQUIRE(ok);

					*bar->open_write() << "Lorem ipsum";
					CHECK(foo->insert("bar", bar));
				}

				auto bar_f = foo->next("bar");
				REQUIRE(nullptr != bar_f);

				auto bar = std::dynamic_pointer_cast<vfs::impl::RegularFile>(bar_f);
				REQUIRE(nullptr != bar);

				std::string line;
				std::getline(*bar->open_read(), line);
				CHECK("Lorem ipsum" == line);
			}

			SECTION("insert a directory into the directory") {
				auto [foo, ok] = sandbox->emplace_directory("foo");
				REQUIRE(ok);

				{
					auto [bar, ok1] = foo->emplace_directory("bar");
					REQUIRE(ok1);

					auto [baz, ok2] = bar->emplace_regular_file("baz");
					REQUIRE(ok2);

					*baz->open_write() << "Lorem ipsum";
				}

				auto bar_f = foo->next("bar");
				REQUIRE(nullptr != bar_f);

				auto bar = std::dynamic_pointer_cast<vfs::impl::Directory>(bar_f);
				REQUIRE(nullptr != bar);

				auto baz_f = bar->next("baz");
				REQUIRE(nullptr != baz_f);

				auto baz = std::dynamic_pointer_cast<vfs::impl::RegularFile>(baz_f);
				REQUIRE(nullptr != baz);

				std::string line;
				std::getline(*baz->open_read(), line);
				CHECK("Lorem ipsum" == line);
			}

			SECTION("iterate a directory") {
				auto [foo, ok] = sandbox->emplace_directory("foo");
				REQUIRE(ok);

				CHECK(foo->empty());
				CHECK(foo->end() == foo->begin());

				foo->emplace_directory("bar");
				foo->emplace_regular_file("baz");
				CHECK(not foo->empty());
				CHECK(foo->end() != foo->begin());

				std::unordered_map<std::string, std::shared_ptr<vfs::impl::File>> files;
				for(auto const& [name, file]: *foo) {
					files.insert(std::make_pair(name, file));
				}

				CHECK(2 == files.size());
				REQUIRE(files.contains("bar"));
				REQUIRE(files.contains("baz"));

				CHECK(nullptr != std::dynamic_pointer_cast<vfs::impl::Directory>(files.at("bar")));
				CHECK(nullptr != std::dynamic_pointer_cast<vfs::impl::RegularFile>(files.at("baz")));
			}
		}
	}
};

}  // namespace suites
}  // namespace testing
