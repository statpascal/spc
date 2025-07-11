#include "pcodeops.hpp"

namespace statpascal {

const std::size_t TPCode::scalarTypeSizes [static_cast<std::size_t> (TPCode::TScalarTypeCode::count)] =
  {8, 4, 2, 1, 1, 2, 4, 4, 8};
  
const std::string TPCode::scalarTypeNames [static_cast<std::size_t> (TPCode::TScalarTypeCode::count)] =
  {"s64", "s32", "s16", "s8", "u8", "u16", "u32", "single", "real"};

}
