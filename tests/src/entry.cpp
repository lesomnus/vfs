#include <memory>
#include <string>
#include <utility>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/entry.hpp>
#include <vfs/impl/file.hpp>

vfs::impl::VStorage storage;

namespace {

template<typename Entry>
std::shared_ptr<Entry> next_insert_(vfs::impl::DirectoryEntry& entry, std::string const& name, std::shared_ptr<vfs::impl::File> file) {
	entry.insert(name, std::move(file));
	return entry.next(name)->must_be<Entry>();
}

}  // namespace

TEST_CASE("Entry") {
	// /
	// + foo/
	//   + bar
	auto root = vfs::impl::DirectoryEntry::make_root();
	auto foo  = next_insert_<vfs::impl::DirectoryEntry>(*root, "foo", storage.make_directory());
	auto bar  = next_insert_<vfs::impl::RegularFileEntry>(*foo, "bar", storage.make_regular_file());

	SECTION("::name") {
		CHECK("foo" == foo->name());
		CHECK("bar" == bar->name());
	}

	SECTION("::holds_same_file_with") {
		CHECK(foo->holds_same_file_with(*foo));
		CHECK(bar->holds_same_file_with(*bar));
		CHECK(not foo->holds_same_file_with(*bar));
	}

	SECTION("::prev") {
		CHECK(root == root->prev());
		CHECK(root == foo->prev());
		CHECK(foo == bar->prev());
	}

	SECTION("::root") {
		CHECK(root == root->root());
		CHECK(root == foo->root());
		CHECK(root == bar->root());
	}

	SECTION("::path") {
		CHECK("/" == root->path());
		CHECK("/foo" == foo->path());
		CHECK("/foo/bar" == bar->path());
	}

	SECTION("::must_be") {
		CHECK_NOTHROW([&] {
			foo->must_be<vfs::impl::DirectoryEntry>();
			bar->must_be<vfs::impl::RegularFileEntry>();
		});
	}
}

TEST_CASE("DirectoryEntry") {
	// /
	// + foo/
	//   + bar
	auto root = vfs::impl::DirectoryEntry::make_root();
	auto foo  = next_insert_<vfs::impl::DirectoryEntry>(*root, "foo", storage.make_directory());
	auto bar  = next_insert_<vfs::impl::RegularFileEntry>(*foo, "bar", storage.make_regular_file());

	SECTION("::next") {
		CHECK(foo->holds_same_file_with(*root->next("foo")));
		CHECK(bar->holds_same_file_with(*foo->next("bar")));
	}

	SECTION("::navigate") {
		SECTION("to existing entry") {
			CHECK(bar->holds_same_file_with(*root->navigate("foo/bar")));
			CHECK(bar->holds_same_file_with(*root->navigate("../../foo/bar")));
			CHECK(foo->holds_same_file_with(*foo->navigate(".")));
			CHECK(root->holds_same_file_with(*foo->navigate("..")));
			CHECK(root->holds_same_file_with(*foo->navigate("./..")));
			CHECK(root->holds_same_file_with(*root->navigate("foo/..")));
		}

		SECTION("to entry that does not exist") {
			std::filesystem::path p = "foo/bar/baz";

			std::error_code ec;
			auto const [entry, it] = root->navigate(p.begin(), p.end(), ec);
			CHECK(bar->holds_same_file_with(*entry));
			CHECK("baz" == *it);
			CHECK(std::errc::not_a_directory == ec);
		}
	}
}

TEST_CASE("SymlinkEntry") {
	// /
	// + foo/
	//   + bar/
	//     + root_a -> /
	//     + parent -> ..
	//   + root_b -> ./bar/root_a
	// + foobar -> /foo/bar
	auto root = vfs::impl::DirectoryEntry::make_root();
	auto foo  = next_insert_<vfs::impl::DirectoryEntry>(*root, "foo", storage.make_directory());
	auto bar  = next_insert_<vfs::impl::DirectoryEntry>(*foo, "bar", storage.make_directory());

	auto root_a = next_insert_<vfs::impl::SymlinkEntry>(*bar, "root_a", storage.make_symlink("/"));
	auto parent = next_insert_<vfs::impl::SymlinkEntry>(*bar, "parent", storage.make_symlink(".."));
	auto root_b = next_insert_<vfs::impl::SymlinkEntry>(*foo, "root_b", storage.make_symlink("./bar/root_a"));
	auto foobar = next_insert_<vfs::impl::SymlinkEntry>(*root, "foobar", storage.make_symlink("/foo/bar"));

	SECTION("::path()") {
		CHECK("/foo/bar/root_a" == root_a->path());
		CHECK("/foo/root_b" == root_b->path());
		CHECK("/foo/bar/parent" == parent->path());
		CHECK("/foobar" == foobar->path());
	}

	SECTION("::follow()") {
		CHECK(root->holds_same_file_with(*root_a->follow()));
		CHECK(root_a->holds_same_file_with(*root_b->follow()));
		CHECK(foo->holds_same_file_with(*parent->follow()));
		CHECK(bar->holds_same_file_with(*foobar->follow()));
	}

	SECTION("::folow_continue()") {
		CHECK(root->holds_same_file_with(*root_a->follow_chain()));
		CHECK(root->holds_same_file_with(*root_b->follow_chain()));
		CHECK(foo->holds_same_file_with(*parent->follow_chain()));
		CHECK(bar->holds_same_file_with(*foobar->follow_chain()));
	}

	SECTION("Directory::get(std::filesystem::path)") {
		// Symlink can be get
		CHECK(root_a->holds_same_file_with(*root->navigate("/foo/bar/root_a")));

		// Symlink is followed if there is next entry in path.
		CHECK(root->holds_same_file_with(*root->navigate("/foo/bar/root_a/")));
		CHECK(bar->holds_same_file_with(*root->navigate("/foo/bar/parent/bar")));

		// Path can contains multiple symlinks
		CHECK(parent->holds_same_file_with(*root->navigate("/foo/bar/parent/bar/parent")));

		// Continued symlinks are followed.
		// Note that `root_b` it linked to `root_a` but `root` is followd
		// since `root_a` is linked to `root`.
		CHECK(root_b->holds_same_file_with(*root->navigate("/foobar/parent/root_b")));
		CHECK(root->holds_same_file_with(*root->navigate("/foobar/parent/root_b/")));

		// Path to symlink is not preserved.
		CHECK(bar->holds_same_file_with(*root->navigate("/foobar/../bar")));
	}
}
