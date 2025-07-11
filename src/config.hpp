/** config.hpp
*/

#pragma once

#include <cstddef>
#include <string>
#include <cstdint>

namespace statpascal {

struct TConfig {
    static const std::size_t alignment = std::min<std::size_t> (std::alignment_of<std::max_align_t> (), 8);
    static const std::size_t setwords = 4;
    static const std::size_t setLimit = setwords * 8 * sizeof (std::int64_t);
    static const std::string globalRuntimeDataPtr; //  = "__globalruntimedata";
    static const std::string binFileType; // = "__bin_file_type";
};

}