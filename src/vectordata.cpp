#include "vectordata.hpp"
#include "anymanager.hpp"

namespace statpascal {

TVectorDataPtr &TVectorDataPtr::operator = (TVectorDataPtr other) {
    return swap (other);
}

TVectorDataPtr &TVectorDataPtr::swap (TVectorDataPtr &other) {
    std::swap (vectorData, other.vectorData);
    return *this;
}

void TVectorDataPtr::copyOnWrite () {
    if (vectorData && vectorData->refcount > 1) {
        --vectorData->refcount;
        TVectorData *copy = static_cast<TVectorData *> (malloc (sizeof (TVectorData) + vectorData->size * vectorData->count));
        new (copy) TVectorData (*vectorData);
        vectorData = copy;
    }
}

TVectorData::TVectorData (const TVectorData &other):
  size (other.size), count (other.count), refcount (1), anyManager (other.anyManager) {
    if (anyManager)
        for (std::size_t i = 0; i < count; ++i)
            setElement (i, &other.get<unsigned char> (i));
    else
        memcpy (data, other.data, size * count);
}

void TVectorData::setElement (std::size_t index, const void *src) {
    unsigned char *dest = &data [index * size];
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