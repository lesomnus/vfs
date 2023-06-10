#include <concepts>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <catch2/catch_template_test_macros.hpp>

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
};

}  // namespace testing

class TestVFile: public testing::TestFileFixture {
   public:
	std::shared_ptr<vfs::impl::Storage> make() {
		return std::make_shared<vfs::impl::VStorage>();
	}
};

METHOD_AS_TEST_CASE(testing::TestFile<TestVFile>::test, "VFile");

class TestOsFile: public testing::TestFileFixture {
   public:
	std::shared_ptr<vfs::impl::Storage> make() {
		return std::make_shared<vfs::impl::OsStorage>();
	}
};

METHOD_AS_TEST_CASE(testing::TestFile<TestOsFile>::test, "OsFile");
