#pragma once

#include <memory>

#include "vfs/fs.hpp"

namespace vfs {

class SysFs: public Fs {
   public:
	std::shared_ptr<std::iostream> open(char const* filename, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) override;

	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) override;
	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) override;

	bool exists(std::filesystem::file_status s) noexcept override;
	bool exists(std::filesystem::path const& p) override;
	bool exists(std::filesystem::path const& p, std::error_code& ec) noexcept override;
};

}  // namespace vfs
