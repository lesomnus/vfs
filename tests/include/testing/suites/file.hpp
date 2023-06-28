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
			SECTION("::operator==") {
				auto const [foo, ok1] = sandbox->emplace_regular_file("foo");
				REQUIRE(ok1);
				CHECK(*foo == *foo);

				auto const [bar, ok2] = sandbox->emplace_regular_file("bar");
				REQUIRE(ok2);
				CHECK(*foo != *bar);

				REQUIRE(sandbox->link("baz", bar));

				auto const baz = sandbox->next("baz");
				REQUIRE(nullptr != baz);
				CHECK(*bar == *baz);
			}
		}

		SECTION("RegularFile") {
			SECTION("::size") {
				auto const [f, ok] = sandbox->emplace_regular_file("foo");
				REQUIRE(ok);
				CHECK(0 == f->size());

				constexpr char quote[] = "Lorem ipsum";
				*f->open_write() << quote;
				CHECK(sizeof(quote) - 1 == f->size());
			}

			SECTION("::last_write_time") {
				auto const [f, ok] = sandbox->emplace_regular_file("foo");
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
			SECTION("::empty") {
				CHECK(sandbox->empty());

				auto [foo, ok] = sandbox->emplace_directory("foo");
				REQUIRE(ok);
				CHECK(foo->empty());
				CHECK(not sandbox->empty());
			}

			SECTION("::contains") {
				REQUIRE(sandbox->emplace_regular_file("foo").second);
				REQUIRE(sandbox->emplace_directory("bar").second);

				CHECK(sandbox->contains("foo"));
				CHECK(sandbox->contains("bar"));
				CHECK(not sandbox->contains("baz"));
			}

			SECTION("::next") {
				{
					auto const [foo, ok] = sandbox->emplace_regular_file("foo");
					REQUIRE(ok);

					*foo->open_write() << "Lorem ipsum";
				}

				auto const foo_f = sandbox->next("foo");
				auto const foo_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(foo_f);
				REQUIRE(nullptr != foo_r);

				std::string line;
				std::getline(*foo_r->open_read(), line);
				CHECK("Lorem ipsum" == line);
			}

			SECTION("::emplace_regular_file") {
				auto const [foo, ok1] = sandbox->emplace_regular_file("foo");
				REQUIRE(ok1);

				{
					auto const [foo_, ok] = sandbox->emplace_regular_file("foo");
					CHECK(not ok);
					REQUIRE(nullptr != foo_);
					CHECK(*foo == *foo_);
				}

				{
					auto const [foo_, ok] = sandbox->emplace_symlink("foo", "/");
					CHECK(not ok);
					CHECK(nullptr == foo_);
				}

				{
					auto const [foo_, ok] = sandbox->emplace_directory("foo");
					CHECK(not ok);
					CHECK(nullptr == foo_);
				}
			}

			SECTION("::emplace_symlink") {
				auto const [foo, ok1] = sandbox->emplace_symlink("foo", "/");
				REQUIRE(ok1);

				{
					auto const [foo_, ok] = sandbox->emplace_regular_file("foo");
					CHECK(not ok);
					CHECK(nullptr == foo_);
				}

				{
					auto const [foo_, ok] = sandbox->emplace_symlink("foo", "/");
					CHECK(not ok);
					REQUIRE(nullptr != foo_);
					CHECK(*foo == *foo_);
				}

				{
					auto const [foo_, ok] = sandbox->emplace_directory("foo");
					CHECK(not ok);
					CHECK(nullptr == foo_);
				}
			}
			SECTION("::emplace_directory") {
				auto const [foo, ok1] = sandbox->emplace_directory("foo");
				REQUIRE(ok1);

				{
					auto const [foo_, ok] = sandbox->emplace_regular_file("foo");
					CHECK(not ok);
					CHECK(nullptr == foo_);
				}

				{
					auto const [foo_, ok] = sandbox->emplace_symlink("foo", "/");
					CHECK(not ok);
					CHECK(nullptr == foo_);
				}

				{
					auto const [foo_, ok] = sandbox->emplace_directory("foo");
					CHECK(not ok);
					REQUIRE(nullptr != foo_);
					CHECK(*foo == *foo_);
				}
			}

			SECTION("::erase") {
				auto const [foo, ok] = sandbox->emplace_directory("foo");
				REQUIRE(ok);
				REQUIRE(foo->emplace_regular_file("bar").second);
				REQUIRE(foo->emplace_directory("baz").second);

				auto const cnt = sandbox->erase("foo");
				CHECK(3 == cnt);
				CHECK(nullptr == sandbox->next("foo"));
			}

			SECTION("::clear") {
				auto const [foo, ok] = sandbox->emplace_directory("foo");
				REQUIRE(ok);
				REQUIRE(foo->emplace_regular_file("bar").second);
				REQUIRE(foo->emplace_directory("baz").second);
				REQUIRE(sandbox->emplace_directory("qux").second);

				auto const cnt = sandbox->clear();
				CHECK(4 == cnt);
				CHECK(sandbox->empty());
				CHECK(nullptr == sandbox->next("foo"));
				CHECK(nullptr == sandbox->next("qux"));
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
