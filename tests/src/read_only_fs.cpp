#include <filesystem>
#include <functional>
#include <system_error>

#include <catch2/catch_template_test_macros.hpp>

#include <vfs/fs.hpp>

TEST_CASE("ReadOnlyFsProxy") {
	namespace fs = std::filesystem;

	auto origin   = vfs::make_mem_fs();
	auto readonly = vfs::make_read_only_fs(*origin);

	SECTION("non-const function fails") {
		std::error_code ec;

		auto test = [&](std::function<void()> const& f) {
			ec.clear();
			try {
				f();
				return ec;
			} catch(fs::filesystem_error const& error) {
				return error.code();
			}
		};

		CHECK(std::errc::read_only_file_system == test([&] { readonly->open_write("foo"); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->canonical("foo", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->weakly_canonical("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->copy("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->copy_file("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->create_directory("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->create_directory("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->create_directories("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->create_hard_link("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->create_symlink("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->current_path(ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->equivalent("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->file_size("foo", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->hard_link_count("foo", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->last_write_time("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->last_write_time("foo", fs::file_time_type{}, ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->permissions("foo", fs::perms::all, ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->read_symlink("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->remove("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->remove_all("foo", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->rename("foo", "bar", ec); }));
		CHECK(std::errc::read_only_file_system == test([&] { readonly->resize_file("foo", 42, ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->space("foo", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->status("foo", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->symlink_status("foo", ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->temp_directory_path(ec); }));
		CHECK(std::errc::read_only_file_system != test([&] { readonly->is_empty("foo", ec); }));
	}
}
