/** \file external.hpp
*/

#pragma once

#include <vector>
#include <string>

namespace statpascal {

class TPCodeMemory; class TPCodeInterpreter; class TRuntimeData;

// enum class TExternalType {ExtInt8, ExtInt16, ExtInt32, ExtInt64, ExtPointer, ExtSingle, ExtDouble, ExtSet, ExtVoid};

using TFuncPtr = void (*) ();

/*

struct TCallbackDescription {
    TCallbackDescription ();
    TCallbackDescription (TExternalType returnType, std::vector<TExternalType> parameter);
    TCallbackDescription (const TCallbackDescription &) = default;

    bool isValid;
    ffi_type *resultType;
    std::vector<ffi_type *> parameter;
};

*/

class TExternalRoutine final {
public:
    TExternalRoutine (const std::string &libName, const std::string &routineName, bool useFFI);
    TExternalRoutine (const TExternalRoutine &) = delete;
    TExternalRoutine (TExternalRoutine &&) = default;
    TExternalRoutine &operator = (const TExternalRoutine &) = delete;
    TExternalRoutine &operator = (TExternalRoutine &&) = default;
    ~TExternalRoutine ();

/*    
    void addParameter (ssize_t offset, TExternalType);
    void addCallback (ssize_t offset, const TCallbackDescription &);
    void setResult (ssize_t offset, TExternalType);
    void setParameterOffset (ssize_t);
    
    void callFFI (void *parameterStart, TPCodeInterpreter &);
    void callDirect (void *parameterStart, TRuntimeData *);
*/
    bool load ();
    
    TFuncPtr getFuncPtr () { return func; }
//    ssize_t getParameterOffset () { return parameterOffset; }
    
private:
    std::string libName, routineName;
//    bool useFFI;
    void *libHandle;
    void (*func) ();
//    ffi_cif cif;
//    std::vector<ssize_t> offsets;
//    std::vector<ffi_type *> types;
//    ssize_t parameterOffset; 	// for direct calls
//    bool hasCallback;
//    std::vector<TCallbackDescription> callbackDescriptions;
    
//    ssize_t resultOffset;
//    ffi_type *resultType;
//    bool useTempResult;
//    std::size_t tempResultSize;
};

/*

class TCallbackData {
public:
    TCallbackData (const TCallbackDescription &callbackDescription, TPCodeMemory &pcodeMemory, std::size_t pcodeAddress);
    ~TCallbackData ();

    TFuncPtr getFunctionPtr () { return reinterpret_cast<void (*) ()> (funcptr); }
    void *getFuncPtrPtr ();
    
    const TCallbackDescription &getCallbackDescription () const;
    std::size_t getPcodeAddress () const;
    
private:
    TCallbackDescription callbackDescription;
    TPCodeMemory &pcodeMemory;
    std::size_t pcodeAddress;
    
    ffi_cif cif;
    ffi_closure *closure;
    void *funcptr;
    
    void handleCallback (void *ret, void *args []);
    static void callback (ffi_cif *cif, void *ret, void *args [], void *userdata);
};

inline void *TCallbackData::getFuncPtrPtr () {
    return &funcptr;
}

inline const TCallbackDescription &TCallbackData::getCallbackDescription () const {
    return callbackDescription;
}

inline std::size_t TCallbackData::getPcodeAddress () const {
    return pcodeAddress;
}
    
inline void TExternalRoutine::callDirect (void *parameterStart, TRuntimeData *p) {
    reinterpret_cast<void (*) (void *, TRuntimeData *)> (func) (static_cast<unsigned char *> (parameterStart) + parameterOffset, p);
}

*/

}
