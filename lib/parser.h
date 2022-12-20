#pragma once

#include "OMFL.h"
#include <filesystem>
#include <istream>

namespace omfl {

    section parse(const std::filesystem::path& path);
    section parse(const std::string& str);

}// namespace