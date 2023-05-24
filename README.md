# vfs

Virtual File System that provides interface of `std::filesystem`.

## Example

```cpp
#include <cassert>
#include <memory>
#include <string>

#include <vfs.hpp>

void work(vfs::Fs& fs) {
	fs.create_directories("foo/bar");
	fs.create_symlink("foo/bar", "baz");

	*fs.open_write("baz/food") << "Royale with cheese";
}

int main(int argc, char* argv[]) {
	auto sandbox = vfs::make_vfs();
	work(*sandbox);

	std::string line;
	std::getline(*sandbox->open_read("foo/bar/food"), line);
	
	assert(line == "Royale with cheese");

	return 0;
}
```

## Features

### File Systems
- `vfs::make_os_fs` Proxy of `std::filesystem`.
- `vfs::make_vfs` Basic file system that is not thread-safe and does not consider permissions.
- 🏗️ `vfs::make_strict_vfs` Basic file system with permissions.

### Utilities
- 🏗️ `vfs::copy` Copies a file between file systems.
- 🏗️ `vfs::chroot` Changes the root directory.
- 🏗️ `vfs::with_user` Switches user.
- 🏗️ `vfs::with_mem_storage` Stores files in the memory.


## About Current Working Directory

```cpp
#include <cassert>
#include <filesystem>

#include <vfs.hpp>

int main(int argc, char* argv[]) {
	// Assume a directory "/tmp/foo/" and a regular file "/tmp/bar/foo" exist.
	{
		namespace fs = std::filesystem;

		fs::current_path("/tmp");
		assert(fs::is_directory("foo")); // References "/tmp/foo/".

		fs::current_path("/tmp/bar");
		assert(not fs::is_directory("foo")); // References "/tmp/bar/foo".
	}

	{
		auto const fs = vfs::make_os_fs();

		auto const tmp = fs->current_path("/tmp");
		assert(tmp->is_directory("foo")); // References "/tmp/foo/".

		auto const bar = fs->current_path("/tmp/bar");
		assert(not bar->is_directory("foo")); // References "/tmp/bar/foo".
		assert(tmp->is_directory("foo"));     // References "/tmp/foo/".
	}

	return 0;
}
```

Standard `std::filesystem::current_path(std::filesystem::path const& p)` changes the Current Working Directory (CWD), which can have an impact on any code in the current process that relies on relative paths, potentially causing unintended results.

In *vfs*, since the file system is an object, each `vfs::Fs` holds its own CWD. `vfs::Fs::current_path(...)` returns a new instance that references the same file system but has a different CWD, instead of replacing the CWD of the existing instance.