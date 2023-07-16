#include <filesystem>
#include <memory>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/mem_file.hpp>
#include <vfs/impl/union_file.hpp>

#include "testing.hpp"

namespace fs = std::filesystem;

TEST_CASE("UnionDirectory") {
	auto upper = std::make_shared<vfs::impl::MemDirectory>(0, 0);
	auto lower = std::make_shared<vfs::impl::MemDirectory>(0, 0);

	// /
	// + foo
	// + bar/
	// + baz/
	//   + qux
	auto [foo, _a] = lower->emplace_regular_file("foo");
	auto [bar, _b] = lower->emplace_directory("bar");
	auto [qux, _c] =
	    lower->emplace_directory("baz")
	        .first->emplace_regular_file("qux");

	*foo->open_write() << testing::QuoteA;
	*qux->open_write() << testing::QuoteB;

	auto root = std::make_shared<vfs::impl::UnionDirectory>(upper, lower);

	REQUIRE(upper->empty());

	SECTION("access regular file on lower") {
		auto next = root->next("foo");
		REQUIRE(nullptr != next);
		CHECK(fs::file_type::regular == next->type());

		auto next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
		REQUIRE(nullptr != next_r);
		CHECK(testing::QuoteA == testing::read_all(*next_r->open_read()));
	}

	SECTION("access directory on lower") {
		auto next = root->next("baz");
		REQUIRE(nullptr != next);
		CHECK(fs::file_type::directory == next->type());

		auto next_d = std::dynamic_pointer_cast<vfs::impl::Directory>(next);
		REQUIRE(nullptr != next_d);
		CHECK(not next_d->empty());
		CHECK(next_d->contains("qux"));
	}

	SECTION("clear on upper") {
		auto const cnt = root->clear();
		CHECK(4 == cnt);
		CHECK(root->empty());
		CHECK(not root->contains("foo"));
		CHECK(not root->contains("bar"));
		CHECK(not root->contains("baz"));

		CHECK(not lower->empty());
		CHECK(lower->contains("foo"));
		CHECK(lower->contains("bar"));
		CHECK(lower->contains("baz"));

		auto next = lower->next("foo");
		REQUIRE(nullptr != next);
		CHECK(fs::file_type::regular == next->type());

		auto next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
		REQUIRE(nullptr != next_r);
		CHECK(testing::QuoteA == testing::read_all(*next_r->open_read()));
	}

	SECTION("upper file hides lower file") {
		std::string name;
		SECTION("that is regular file") {
			name = "foo";
		}
		SECTION("that is directory") {
			name = "bar";
		}

		{
			auto [next_r, ok] = upper->emplace_regular_file(name);
			REQUIRE(ok);

			*next_r->open_write() << testing::QuoteC;
		}

		auto next = root->next(name);
		REQUIRE(nullptr != next);
		CHECK(fs::file_type::regular == next->type());

		auto next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
		REQUIRE(nullptr != next_r);
		CHECK(testing::QuoteC == testing::read_all(*next_r->open_read()));
	}

	SECTION("upper directory does not hide lower directory") {
		{
			auto [next_d, ok1] = upper->emplace_directory("baz");
			REQUIRE(nullptr != next_d);
			REQUIRE(ok1);

			auto [next_r, ok2] = next_d->emplace_regular_file("egg");
			REQUIRE(nullptr != next_r);
			REQUIRE(ok2);

			*next_r->open_write() << testing::QuoteC;
		}

		CHECK(root->contains("baz"));

		auto next = root->next("baz");
		REQUIRE(nullptr != next);

		auto next_d = std::dynamic_pointer_cast<vfs::impl::Directory>(next);
		REQUIRE(nullptr != next_d);

		CHECK(next_d->contains("qux"));
		CHECK(next_d->contains("egg"));

		{
			auto next = next_d->next("qux");
			REQUIRE(nullptr != next);

			auto next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
			REQUIRE(nullptr != next_r);
			CHECK(testing::QuoteB == testing::read_all(*next_r->open_read()));
		}

		{
			auto next = next_d->next("egg");
			REQUIRE(nullptr != next);

			auto next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
			REQUIRE(nullptr != next_r);
			CHECK(testing::QuoteC == testing::read_all(*next_r->open_read()));
		}
	}

	SECTION("copy on write") {
		auto next = root->next("baz");
		REQUIRE(nullptr != next);

		auto next_d = std::dynamic_pointer_cast<vfs::impl::Directory>(next);
		REQUIRE(nullptr != next_d);

		next = next_d->next("qux");
		REQUIRE(nullptr != next);

		auto next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
		REQUIRE(nullptr != next_r);

		CHECK(not upper->contains("baz"));
		CHECK(testing::QuoteB == testing::read_all(*next_r->open_read()));
		CHECK(not upper->contains("baz"));

		CHECK(not upper->contains("baz"));
		*next_r->open_write(std::ios_base::app) << testing::QuoteC;
		CHECK(upper->contains("baz"));

		next = upper->next("baz");
		REQUIRE(nullptr != next);

		next_d = std::dynamic_pointer_cast<vfs::impl::Directory>(next);
		REQUIRE(nullptr != next_d);

		next = next_d->next("qux");
		REQUIRE(nullptr != next);

		next_r = std::dynamic_pointer_cast<vfs::impl::RegularFile>(next);
		REQUIRE(nullptr != next_r);

		CHECK((std::string(testing::QuoteB) + std::string(testing::QuoteC)) == testing::read_all(*next_r->open_read()));
	}

	SECTION("lower is looked when emplacing regular file") {
		REQUIRE(not upper->contains("foo"));
		REQUIRE(lower->contains("foo"));

		auto [file, ok] = root->emplace_regular_file("foo");
		CHECK(nullptr != file);
		CHECK(!ok);
	}

	SECTION("lower is looked when emplacing directory") {
		REQUIRE(not upper->contains("bar"));
		REQUIRE(lower->contains("bar"));

		auto [file, ok] = root->emplace_directory("bar");
		CHECK(nullptr != file);
		CHECK(!ok);
	}
}
