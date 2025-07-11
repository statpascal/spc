/** \file runtime.hpp
*/

#pragma once

#include <unordered_map>
#include <deque>

#include "anymanager.hpp"
#include "anyvalue.hpp"
#include "filehandler.hpp"

namespace statpascal {

class TPCodeMemory;

class TRuntimeData final {
public:
    TRuntimeData ();
    ~TRuntimeData ();
    
    std::size_t registerAnyManager (TAnyManager *);
    TAnyManager *getAnyManager (std::size_t index);
    
    std::size_t registerStringConstant (const std::string &);
    TAnyValue getStringConstant (std::size_t index);
    
    std::size_t registerData (const void *, std::size_t);
    void *getData (std::size_t index);
    
    void *allocateMemory (std::size_t count, std::size_t size, std::size_t anyManagerIndex);
    void releaseMemory (void *p);
    
    TTextFileBaseHandler &getTextFileBaseHandler (std::size_t index);
    TBinaryFileHandler &getBinaryFileHandler (std::size_t index);
    TFileHandler &getFileHandler (std::size_t index);
    std::size_t registerFileHandler (TFileHandler *);
    void closeFileHandler (std::size_t index);
    
    void clearArgv ();
    void appendArgv (const std::string &s);
    std::int64_t getArgc ();
    statpascal::TAnyValue getArgv (std::int64_t n);
    
    // TODO: needed for thread creation - find better solution
    void setPCodeMemory (TPCodeMemory *);
    TPCodeMemory *getPCodeMemory ();
    
private:
    struct TArrayData {
        std::size_t size, count;
        std::size_t anyManagerIndex;
    };
    typedef std::unordered_map<void *, TArrayData> TDynamicArrayMap;
    
    TDynamicArrayMap dynamicArrayMap;
    std::vector<TAnyManager *> anyManagers;
    std::deque<TAnyValue> stringPool;
    std::vector<std::vector<unsigned char>> dataPool;
    std::vector<TFileHandler *> fileHandler;
    std::vector<std::string> argvector;
    
    TPCodeMemory *pcodeMemory;
};

// runtime calls

extern "C" {

void rt_copy_mem (void *dst, const void *src, std::size_t size, std::size_t anyManagerIndex, bool deleteDst, TRuntimeData *);
void *rt_alloc_mem (std::size_t count, std::size_t size, std::size_t anyManagerIndex, TRuntimeData *);
void rt_free_mem (void *, TRuntimeData *);
void rt_destroy_mem (void *, std::size_t anyManagerIndex, TRuntimeData *);

char *rt_str_index (statpascal::TAnyValue s, std::size_t index);

std::int64_t rt_argc (TRuntimeData *);
void rt_argv (std::int64_t n, TRuntimeData *, statpascal::TAnyValue *s);

void rt_halt (std::int64_t, TRuntimeData *);

}

}