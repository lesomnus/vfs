#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>

#include <vfs/fs.hpp>

namespace testing {

constexpr std::string_view QuoteA = "Lorem ipsum dolor sit amet";
constexpr std::string_view QuoteB = "Ut enim ad minim veniam";

std::shared_ptr<vfs::Fs> cd_temp_dir(vfs::Fs& fs);

std::string read_all(std::istream& in);

}  // namespace testing
