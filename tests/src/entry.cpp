#include <catch2/catch_template_test_macros.hpp>

#include <vfs/impl/entry.hpp>
#include <vfs/impl/file.hpp>

// TEST_CASE("Directory") {
// 	// /
// 	// + foo
// 	// ? bar

// 	auto root = vfs::impl::DirectoryEntry::make_root();
// 	auto foo  = std::make_shared<vfs::impl::EmptyFile>(*root);
// 	auto bar  = std::make_shared<vfs::impl::EmptyFile>(*root);
// 	root->insert(std::make_pair("foo", foo));
// 	root->insert(std::make_pair("bar", bar));

// 	SECTION("::is_root()") {
// 		REQUIRE(not foo->is_root());
// 		REQUIRE(not bar->is_root());
// 	}

// 	SECTION("::prev()") {
// 		REQUIRE(root == foo->prev());
// 		REQUIRE_THROWS_AS(bar->prev(), std::filesystem::filesystem_error);
// 	}

// 	SECTION("::root()") {
// 		REQUIRE(root == foo->root());
// 		REQUIRE_THROWS_AS(bar->root(), std::filesystem::filesystem_error);
// 	}

// 	SECTION("::path()") {
// 		REQUIRE("/foo" == foo->path());
// 		REQUIRE_THROWS_AS(bar->path(), std::filesystem::filesystem_error);
// 	}
// }

TEST_CASE("DirectoryEntry") {
	// /
	// + /foo
	//   + /bar
	auto root = vfs::impl::DirectoryEntry::make_root();
	auto foo  = root->next_insert("foo", std::make_shared<vfs::impl::Directory>(root->typed_file()));
	auto bar  = foo->next_insert("bar", std::make_shared<vfs::impl::Directory>(foo->typed_file()));

	SECTION("::prev()") {
		REQUIRE(root == root->prev());
		REQUIRE(root == foo->prev());
		REQUIRE(foo == bar->prev());
	}

	SECTION("::root()") {
		REQUIRE(root == root->root());
		REQUIRE(root == foo->root());
		REQUIRE(root == bar->root());
	}

	SECTION("::path()") {
		REQUIRE("/" == root->path());
		REQUIRE("/foo" == foo->path());
		REQUIRE("/foo/bar" == bar->path());
	}

	SECTION("::next()") {
		REQUIRE(foo->holds_same_file_with(*root->next("foo")));
		REQUIRE(bar->holds_same_file_with(*foo->next("bar")));
	}

	SECTION("::navigate") {
		SECTION("to existing entry") {
			REQUIRE(bar->holds_same_file_with(*root->navigate("foo/bar")));
			REQUIRE(bar->holds_same_file_with(*root->navigate("../../foo/bar")));
			REQUIRE(foo->holds_same_file_with(*foo->navigate(".")));
			REQUIRE(root->holds_same_file_with(*foo->navigate("..")));
			REQUIRE(root->holds_same_file_with(*foo->navigate("./..")));
			REQUIRE(root->holds_same_file_with(*root->navigate("foo/..")));
		}

		SECTION("to entry that does not exist") {
			std::error_code ec;

			std::filesystem::path p = "foo/bar/baz";

			auto const [entry, it] = root->navigate(p.begin(), p.end(), ec);

			REQUIRE(bar->holds_same_file_with(*entry));
			REQUIRE("baz" == *it);
			REQUIRE(std::errc::no_such_file_or_directory == ec);
		}
	}
}

TEST_CASE("SymlinkEntry") {
	// /
	// + /foo
	//   + /bar
	//     + root_a -> /
	//     + parent -> ..
	//   + root_b -> ./bar/root_a
	// + foobar -> /foo/bar

	auto root = vfs::impl::DirectoryEntry::make_root();
	auto foo  = root->next_insert("foo", std::make_shared<vfs::impl::Directory>(root->typed_file()));
	auto bar  = foo->next_insert("bar", std::make_shared<vfs::impl::Directory>(foo->typed_file()));

	auto root_a = bar->next_insert("root_a", std::make_shared<vfs::impl::Symlink>("/"));
	auto parent = bar->next_insert("parent", std::make_shared<vfs::impl::Symlink>(".."));
	auto root_b = foo->next_insert("root_b", std::make_shared<vfs::impl::Symlink>("./bar/root_a"));
	auto foobar = root->next_insert("foobar", std::make_shared<vfs::impl::Symlink>("/foo/bar"));

	SECTION("::path()") {
		REQUIRE("/foo/bar/root_a" == root_a->path());
		REQUIRE("/foo/root_b" == root_b->path());
		REQUIRE("/foo/bar/parent" == parent->path());
		REQUIRE("/foobar" == foobar->path());
	}

	SECTION("::follow()") {
		REQUIRE(root->holds_same_file_with(*root_a->follow()));
		REQUIRE(root_a->holds_same_file_with(*root_b->follow()));
		REQUIRE(foo->holds_same_file_with(*parent->follow()));
		REQUIRE(bar->holds_same_file_with(*foobar->follow()));
	}

	SECTION("::folow_continue()") {
		REQUIRE(root->holds_same_file_with(*root_a->follow_chain()));
		REQUIRE(root->holds_same_file_with(*root_b->follow_chain()));
		REQUIRE(foo->holds_same_file_with(*parent->follow_chain()));
		REQUIRE(bar->holds_same_file_with(*foobar->follow_chain()));
	}

	SECTION("Directory::get(std::filesystem::path)") {
		// Symlink can be get
		REQUIRE(root_a->holds_same_file_with(*root->navigate("/foo/bar/root_a")));

		// Symlink is followed if there is next entry in path.
		REQUIRE(root->holds_same_file_with(*root->navigate("/foo/bar/root_a/")));
		REQUIRE(bar->holds_same_file_with(*root->navigate("/foo/bar/parent/bar")));

		// Path can contains multiple symlinks
		REQUIRE(parent->holds_same_file_with(*root->navigate("/foo/bar/parent/bar/parent")));

		// Continued symlinks are followed.
		// Note that `root_b` it linked to `root_a` but `root` is folowd
		// since `root_a` is linked to `root`.
		REQUIRE(root_b->holds_same_file_with(*root->navigate("/foobar/parent/root_b")));
		REQUIRE(root->holds_same_file_with(*root->navigate("/foobar/parent/root_b/")));

		// Path to symlink is not preserved.
		REQUIRE(bar->holds_same_file_with(*root->navigate("/foobar/../bar")));
	}
}
