/** config.cpp
*/

#include "config.hpp"

namespace statpascal {

const std::string 
    TConfig::globalRuntimeDataPtr = "__globalruntimedata",
    TConfig::binFileType = "__bin_file_type";
    
std::uint16_t TConfig::startBank = 0;
bool TConfig::omitHeader = false;
    
TConfig::TTarget TConfig::target;

}