#include "runtime.hpp"
#include "runtimeerr.hpp"

#include <cstring>

namespace statpascal {

TRuntimeData::TRuntimeData ():
  pcodeMemory (nullptr) {
    anyManagers.push_back (nullptr);
}

TRuntimeData::~TRuntimeData () {
    for (TAnyManager *p: anyManagers)
        delete p;
    for (TFileHandler *f: fileHandler)
        delete f;
}

std::size_t TRuntimeData::registerAnyManager (TAnyManager *anyManager) {
    anyManagers.push_back (anyManager);
    return anyManagers.size () - 1;
}

TAnyManager *TRuntimeData::getAnyManager (std::size_t index) {
    return anyManagers [index];
}

std::size_t TRuntimeData::registerStringConstant (const std::string &s) {
    for (std::size_t i = 0; i < stringPool.size (); ++i)
        if (stringPool [i].get<std::string> () == s)
            return i;
    stringPool.emplace_back (TAnyValue (s));
    return stringPool.size () - 1;
}

TAnyValue TRuntimeData::getStringConstant (std::size_t index) {
    return stringPool [index];
}

std::size_t TRuntimeData::registerData (const void *p, const std::size_t size) {
    const unsigned char *q = static_cast<const unsigned char *> (p);
    dataPool.push_back (std::vector<unsigned char> (q, q + size));
    return dataPool.size () - 1;
}

void *TRuntimeData::getData (std::size_t index) {
    return dataPool [index].data ();
}

void *TRuntimeData::allocateMemory (std::size_t count, std::size_t size, std::size_t anyManagerIndex) {
    // TODO: Lock
    void *p = std::calloc (count, size);
    if (anyManagerIndex)
        dynamicArrayMap [p] = {size, count, anyManagerIndex};
    return p;
}

void TRuntimeData::releaseMemory (void *p) {
    // TODO: Lock
    TDynamicArrayMap::iterator it = dynamicArrayMap.find (p);
    if (it != dynamicArrayMap.end ()) {
        TArrayData arrayData = it->second;
        unsigned char *q = static_cast<unsigned char *> (p);
        for (std::size_t i = 0; i < arrayData.count; ++i)
            anyManagers [arrayData.anyManagerIndex]->destroy (q + i * arrayData.size);
        dynamicArrayMap.erase (it);
    }
    std::free (p);
}

TFileHandler &TRuntimeData::getFileHandler (std::size_t index) {
    if (index < fileHandler.size () && fileHandler [index])
        return *fileHandler [index];
    throw TRuntimeError (TRuntimeResult::TCode::IOError, "File is not open");
}

TTextFileBaseHandler &TRuntimeData::getTextFileBaseHandler (std::size_t index) {
    return *static_cast<TTextFileBaseHandler *> (&getFileHandler (index));
}

TBinaryFileHandler &TRuntimeData::getBinaryFileHandler (std::size_t index) {
    return *static_cast<TBinaryFileHandler *> (&getFileHandler (index));
}

std::size_t TRuntimeData::registerFileHandler (TFileHandler *fh) {
    for (std::size_t i = 0; i < fileHandler.size (); ++i)
        if (!fileHandler [i]) {
            fileHandler [i] = fh;
            return i;
        }
    fileHandler.push_back (fh);
    return fileHandler.size () - 1;
}

void TRuntimeData::closeFileHandler (std::size_t index) {
    if (index < fileHandler.size ()) {
        delete fileHandler [index];
        fileHandler [index] = nullptr;
    } else
        throw TRuntimeError (TRuntimeResult::TCode::IOError, "File is not open");
}

void TRuntimeData::setPCodeMemory (TPCodeMemory *p) {
    pcodeMemory = p;
}

TPCodeMemory *TRuntimeData::getPCodeMemory () {
    return pcodeMemory;
}

void TRuntimeData::clearArgv () {
    argvector.clear ();
}

void TRuntimeData::appendArgv (const std::string &s) {
    argvector.push_back (s);
}

std::int64_t TRuntimeData::getArgc () {
    return argvector.size () - 1;
}

statpascal::TAnyValue TRuntimeData::getArgv (std::int64_t n) {
    if (n >= 0 && static_cast<std::size_t> (n) < argvector.size ())
        return argvector [n];
    return std::string ();
}

// runtime calls

extern "C" void rt_copy_mem (void *dst, const void *src, std::size_t size, std::size_t anyManagerIndex, bool deleteDst, TRuntimeData *runtimeData) {
//    printf ("rt_copy_mem: %p %p, %lld, %lld, %d, %p\n", dst, src, size, anyManagerIndex, deleteDst, runtimeData);
    TAnyManager *anyManager = runtimeData->getAnyManager (anyManagerIndex);
    if (deleteDst)
        anyManager->destroy (dst);
    std::memcpy (dst, src, size);
    anyManager->copy (src, dst);
}

extern "C" void *rt_alloc_mem (std::size_t count, std::size_t size, std::size_t anyManagerIndex, TRuntimeData *runtimeData) {
    return runtimeData->allocateMemory (count, size, anyManagerIndex);
}

extern "C" void rt_free_mem (void *p, TRuntimeData *runtimeData) {
    runtimeData->releaseMemory (p);
}

extern "C" void rt_init_mem (void *p, std::size_t anyManagerIndex, TRuntimeData *runtimeData) {
    runtimeData->getAnyManager (anyManagerIndex)->init (p);
}

extern "C" void rt_destroy_mem (void *p, std::size_t anyManagerIndex, TRuntimeData *runtimeData) {
    runtimeData->getAnyManager (anyManagerIndex)->destroy (p);
}

extern "C" char *rt_str_index (statpascal::TAnyValue s, std::size_t index) {
    static char n = 0;
    if (s.hasValue ()) 
        // TODO: index check
        return &(s.get<std::string> () [index - 1]);
    else
        return &n;        
}

extern "C" std::int64_t rt_argc (TRuntimeData *runtimeData) {
    return runtimeData->getArgc ();
}

extern "C" void rt_argv (std::int64_t n, TRuntimeData *runtimeData, statpascal::TAnyValue *s) {
    *s = runtimeData->getArgv (n);
}

extern "C" void rt_halt (std::int64_t n, TRuntimeData *) {
    exit (n);
}

}
