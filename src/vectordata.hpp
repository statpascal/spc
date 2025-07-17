/** \file vectordata.hpp
*/

#pragma once

#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <new>
#include <algorithm>

namespace statpascal {

class TAnyManager;

class TVectorData {
public:
    TVectorData (std::size_t elementSize, std::size_t elementCount, TAnyManager *elementAnyManager = nullptr, bool zeroMemory = true);
    
    TVectorData (const TVectorData &);
    ~TVectorData ();
    
    TVectorData &operator = (TVectorData) = delete;
    
    void setElement (std::size_t index, const void *src);
    const void *getElement (std::size_t index) const;
    void *getElement (std::size_t index);
    
    template<typename T> T &get (std::size_t index);
    template<typename T> const T &get (std::size_t index) const;
    
    TAnyManager *getElementAnyManager () const;
    std::size_t getElementSize () const;
    std::size_t getElementCount () const;

private:
    void deleteData ();

    std::size_t size, count;
    TAnyManager *anyManager;
    char *data;
};


inline TVectorData::TVectorData (std::size_t size, std::size_t count, TAnyManager *anyManager, bool zeroMemory):
  size (size), count (count), anyManager (anyManager) {
    data = static_cast<char *> (operator new (size * count));
    if (zeroMemory)
        std::fill (data, data + size * count, 0);
}

inline TVectorData::~TVectorData () {
    if (anyManager)
        deleteData ();
    operator delete (data);
}

inline TAnyManager *TVectorData::getElementAnyManager () const {
    return anyManager;
}

inline std::size_t TVectorData::getElementSize () const {
    return size;
}

inline std::size_t TVectorData::getElementCount () const {
    return count;
}

inline const void *TVectorData::getElement (std::size_t index) const {
    return &data [index * size];
}

inline void *TVectorData::getElement (std::size_t index) {
    return &data [index * size];
}

template<typename T> inline const T &TVectorData::get (std::size_t index) const {
    return *static_cast<const T *> (getElement (index));
}

template<typename T> inline T &TVectorData::get (std::size_t index) {
    return const_cast<T &> (static_cast<const TVectorData *> (this)->get<T> (index));
}

}
