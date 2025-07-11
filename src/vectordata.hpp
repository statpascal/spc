/** \file vectordata.hpp
*/

#pragma once

#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <new>
#include <algorithm>

namespace statpascal {

class TVectorData;
class TAnyManager;

class TVectorDataPtr {
public:
    TVectorDataPtr ();
    TVectorDataPtr (const TVectorDataPtr &);
    explicit TVectorDataPtr (void *, bool incRef = true);
    TVectorDataPtr (std::size_t elementSize, std::size_t elementCount, TAnyManager *elementAnyManager = nullptr, bool zeroMemory = true);
    
    TVectorDataPtr &operator = (TVectorDataPtr);
    
    ~TVectorDataPtr ();

    TVectorDataPtr &swap (TVectorDataPtr &);
    
    void copyOnWrite ();
    bool hasValue () const;
    
    TVectorData &operator * () const;
    TVectorData *operator -> () const;
    
    TVectorData *makeRef () const;
    void releaseRef ();
    
private:
    TVectorData *vectorData;
};


class TVectorData {
public:
    void setElement (std::size_t index, const void *src);
    const void *getElement (std::size_t index) const;
    void *getElement (std::size_t index);
    
    template<typename T> T &get (std::size_t index);
    template<typename T> const T &get (std::size_t index) const;
    
    TAnyManager *getElementAnyManager () const;
    std::size_t getElementSize () const;
    std::size_t getElementCount () const;
    std::size_t getRefCount () const;

private:
    TVectorData (std::size_t elementSize, std::size_t elementCount, TAnyManager *elementAnyManager = nullptr, bool zeroMemory = true);
    TVectorData (const TVectorData &);
    ~TVectorData ();
    
    TVectorData &operator = (TVectorData) = delete;
    
    void deleteData ();

    std::size_t size, count, refcount;
    TAnyManager *anyManager;
    unsigned char data [1];
    
    friend TVectorDataPtr;
};


inline TVectorData::TVectorData (std::size_t size, std::size_t count, TAnyManager *anyManager, bool zeroMemory):
  size (size), count (count), refcount (1), anyManager (anyManager) {
    if (zeroMemory)
        std::fill (data, data + size * count, 0);
}

inline TVectorDataPtr::TVectorDataPtr (std::size_t elementSize, std::size_t elementCount, TAnyManager *elementAnyManager, bool zeroMemory):
  vectorData (static_cast<TVectorData *> (malloc (sizeof (TVectorData) + elementSize * elementCount))) {
    new (vectorData) TVectorData (elementSize, elementCount, elementAnyManager, zeroMemory);
}

inline TVectorData *TVectorDataPtr::operator -> () const {
    return vectorData;
}

inline TVectorData *TVectorDataPtr::makeRef () const {
    if (vectorData)
        ++vectorData->refcount;
    return vectorData;
}

inline void TVectorDataPtr::releaseRef () {
    if (vectorData && !--vectorData->refcount) {
        vectorData->~TVectorData ();
        free (vectorData);
    }
}

inline TVectorData::~TVectorData () {
    if (anyManager)
        deleteData ();
}

inline TAnyManager *TVectorData::getElementAnyManager () const {
    return anyManager;
}

inline std::size_t TVectorData::getElementSize () const {
    return size;
}

inline std::size_t TVectorData::getRefCount () const {
    return refcount;
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

inline TVectorDataPtr::TVectorDataPtr ():
  vectorData (nullptr) {
}

inline TVectorDataPtr::TVectorDataPtr (const TVectorDataPtr &other):
  vectorData (other.makeRef ()) {
}

inline TVectorDataPtr::TVectorDataPtr (void *p, bool incRef):
  vectorData (static_cast<TVectorData *> (p)) {
    if (incRef && vectorData)
        ++vectorData->refcount;
}

inline TVectorDataPtr::~TVectorDataPtr () {
    releaseRef ();
}

inline bool TVectorDataPtr::hasValue () const {
    return !!vectorData;
}

inline TVectorData &TVectorDataPtr::operator * () const {
    return *vectorData;
}

}
