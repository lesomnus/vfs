#include <memory>
#include <string>
#include <utility>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/entry.hpp>
#include <vfs/impl/file.hpp>

TEST_CASE("Entry") {
	// /
	// + foo/
	//   + bar
	auto root = vfs::impl::DirectoryEntry::make_root();
	auto foo  = vfs::impl::DirectoryEntry::make("foo", root, root->emplace_directory("foo"));
	auto bar  = vfs::impl::RegularFileEntry::make("bar", foo, foo->emplace_regular_file("bar"));

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
	auto foo  = vfs::impl::DirectoryEntry::make("foo", root, root->emplace_directory("foo"));
	auto bar  = vfs::impl::RegularFileEntry::make("bar", foo, foo->emplace_regular_file("bar"));

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
	auto foo  = vfs::impl::DirectoryEntry::make("foo", root, root->emplace_directory("foo"));
	auto bar  = vfs::impl::DirectoryEntry::make("bar", foo, foo->emplace_directory("bar"));

	auto root_a = vfs::impl::SymlinkEntry::make("root_a", bar, bar->emplace_symlink("root_a", "/"));
	auto parent = vfs::impl::SymlinkEntry::make("parent", bar, bar->emplace_symlink("parent", ".."));
	auto root_b = vfs::impl::SymlinkEntry::make("root_b", foo, foo->emplace_symlink("root_b", "./bar/root_a"));
	auto foobar = vfs::impl::SymlinkEntry::make("foobar", root, root->emplace_symlink("foobar", "/foo/bar"));

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
