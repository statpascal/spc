#include "vectordata.hpp"
#include "anymanager.hpp"

namespace statpascal {

TVectorData::TVectorData (const TVectorData &other):
  size (other.size), count (other.count), anyManager (other.anyManager) {
    data = static_cast<char *> (operator new (size * count));
    if (anyManager)
        for (std::size_t i = 0; i < count; ++i)
            setElement (i, &other.get<unsigned char> (i));
    else
        memcpy (data, other.data, size * count);
}

void TVectorData::setElement (std::size_t index, const void *src) {
    char *dest = &data [index * size];
    std::memcpy (dest, src, size);
    if (anyManager)
        anyManager->copy (src, dest);
}

void TVectorData::deleteData () {
    if (anyManager)        
        for (std::size_t i = 0; i < count; ++i)
            anyManager->destroy (&get<unsigned char> (i));
}

}