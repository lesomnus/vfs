#include <concepts>
#include <memory>
#include <unordered_set>
#include <vector>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/storage.hpp>

namespace fs = std::filesystem;

namespace testing {

class TestStorageFixture {
   public:
	virtual std::shared_ptr<vfs::impl::Storage> make() = 0;
};

template<std::derived_from<TestStorageFixture> Fixture>
class TestStorage {
   public:
	void test() {
		Fixture fixture;

		std::shared_ptr<vfs::impl::Storage> const storage = fixture.make();

		SECTION("iterate empty directory") {
			auto d = storage->make_directory();
			CHECK(d->empty());
			CHECK(d->end() == d->begin());
		}

		SECTION("iterate nested empty directory") {
			auto foo = storage->make_directory();
			auto bar = storage->make_directory();

			foo->insert("bar", bar);
			for(auto const& [name, file]: *foo) {
				CHECK("bar" == name);
				CHECK(bar == file);
			}
		}

		SECTION("iterate directory") {
			auto d = storage->make_directory();
			d->insert("a", storage->make_regular_file());
			d->insert("b", storage->make_regular_file());

			std::vector<std::string> filenames;
			for(auto const [name, file]: *d) {
				CHECK(fs::file_type::regular == file->type());
				filenames.push_back(name);
			}

			std::unordered_set<std::string> expected{"a", "b"};
			CHECK(expected.size() == filenames.size());

			std::unordered_set<std::string> actual(filenames.begin(), filenames.end());
			CHECK(std::unordered_set<std::string>{"a", "b"} == actual);
		}
	}
};

}  // namespace testing

class TestVStorage: public testing::TestStorageFixture {
   public:
	std::shared_ptr<vfs::impl::Storage> make() {
		return std::make_shared<vfs::impl::VStorage>();
	}
};

METHOD_AS_TEST_CASE(testing::TestStorage<TestVStorage>::test, "VStorage");
