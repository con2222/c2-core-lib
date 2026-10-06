#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace C2Core::File {
std::optional<std::string> readText(const std::string& filepath);
}
