# vfs

[![test](https://github.com/lesomnus/vfs/actions/workflows/test.yaml/badge.svg)](https://github.com/lesomnus/vfs/actions/workflows/test.yaml)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/14de41c183224821af5004302a830441)](https://app.codacy.com/gh/lesomnus/vfs/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/14de41c183224821af5004302a830441)](https://app.codacy.com/gh/lesomnus/vfs/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_coverage)

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
-  `vfs::make_os_fs` Proxy of `std::filesystem`.
-  `vfs::make_vfs` Basic file system that is not thread-safe and does not consider permissions.
-  🏗️ `vfs::make_strict_vfs` Basic file system with permissions.

### Utilities
-  `vfs::Fs::change_root` Changes the root directory.
-  `vfs::Fs::mount` Mounts different file system.
-  `vfs::Fs::copy` Copies a file between file systems.
-  🏗️ `vfs::with_user` Switches user.
-  🏗️ `vfs::with_mem_storage` Stores files in the memory.


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


## Mount

```cpp
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include <vfs.hpp>

int main(int argc, char* argv[]) {
	namespace fs = std::filesystem;

	auto const sandbox = vfs::make_vfs();
	sandbox->create_directories("foo/bar");

	sandbox->mount("foo/bar", *vfs::make_os_fs(), "/tmp");
	*sandbox->open_write("foo/bar/food") << "Royale with cheese";
	asser(sandbox->exists("foo/bar/food"));
	asser(fs::exists("/tmp/food"));

	{
		std::string line;
		std::getline(*sandbox->open_read("foo/bar/food"), line);
		asser(line == "Royale with cheese");
	}

	{
		std::ifstream food("/tmp/food");
		std::string   line;
		std::getline(food, line);
		asser(line == "Royale with cheese");
	}

	sandbox->unmount("foo/bar");
	asser(not sandbox->exists("foo/bar/food"));
	asser(fs::exists("/tmp/food"));

	return 0;
}
```

`Fs::mount` is similar to a bind mount.
It allows you to mount a file from a filesystem to a different path or mount a file from a different filesystem.
