#include "external.hpp"

#include <dlfcn.h>
#include <unistd.h>
#include <alloca.h>
#include <map>

namespace statpascal {

static bool setTypeBuilt = false;
ffi_type *ffi_type_set_fields [TConfig::setwords + 1];
ffi_type ffi_type_set;

ffi_type *const ffiTypes [] = {&ffi_type_uint8, &ffi_type_uint16, &ffi_type_uint32, &ffi_type_uint64, &ffi_type_pointer, &ffi_type_float, &ffi_type_double, &ffi_type_set, &ffi_type_void};

static void buildSetType () {
    ffi_type_set.size = ffi_type_set.alignment = 0;
    ffi_type_set.type = FFI_TYPE_STRUCT;
    ffi_type_set.elements = ffi_type_set_fields;
    for (std::size_t i = 0; i < TConfig::setwords; ++i)
        ffi_type_set_fields [i] = &ffi_type_uint64;
    ffi_type_set_fields [TConfig::setwords] = nullptr;
    setTypeBuilt = true;
}

TCallbackDescription::TCallbackDescription ():
  isValid (false), resultType (nullptr) {
}

TCallbackDescription::TCallbackDescription (TExternalType returnType, std::vector<TExternalType> p):
  isValid (true),
  resultType (ffiTypes [static_cast<std::size_t> (returnType)]) {
    for (TExternalType externalType: p)
        parameter.push_back (ffiTypes [static_cast<std::size_t> (externalType)]);
}


TCallbackData::TCallbackData (const TCallbackDescription &cbd, TPCodeMemory &pcodeMemory, std::size_t pcodeAddress):
  callbackDescription (cbd),
  pcodeMemory (pcodeMemory),
  pcodeAddress (pcodeAddress),
  closure (nullptr) {
    closure = static_cast<ffi_closure *> (ffi_closure_alloc (sizeof (ffi_closure), &funcptr));
    if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, callbackDescription.parameter.size (), callbackDescription.resultType, &callbackDescription.parameter [0]) != FFI_OK) {
        puts ("ffi_prep_cif failed");
        abort ();
    }
    if (ffi_prep_closure_loc (closure, &cif, callback, this, funcptr) != FFI_OK) {
        puts ("ffi_prep_closure_loc failed");
        abort ();
    }
}

TCallbackData::~TCallbackData () {
    if (closure)
        ffi_closure_free (closure);
}

void TCallbackData::handleCallback (void *ret, void **args) {
    TPCodeInterpreter &pcodeInterpreter = pcodeMemory.getPCodeInterpreter ();
    TStack &activeStack = pcodeInterpreter.getStack ();
    if (callbackDescription.resultType != &ffi_type_void)
        activeStack.push (ret);
    for (ffi_type *type: callbackDescription.parameter)
        if (type == &ffi_type_uint8)
            activeStack.push (*static_cast<std::uint8_t *> (*args++));
        else if (type == &ffi_type_uint16)
            activeStack.push (*static_cast<std::uint16_t *> (*args++));
        else if (type == &ffi_type_uint32)
            activeStack.push (*static_cast<std::uint32_t *> (*args++));
        else if (type == &ffi_type_uint64)
            activeStack.push (*static_cast<std::uint64_t *> (*args++));
        else if (type == &ffi_type_pointer)
            activeStack.push (*static_cast<void **> (*args++));
        else if (type == &ffi_type_float)
            activeStack.push (*static_cast<float *> (*args++));
        else if (type == &ffi_type_double)
            activeStack.push (*static_cast<double *> (*args++));
    pcodeInterpreter.callRoutine (pcodeAddress);
}

void TCallbackData::callback (ffi_cif *cif, void *ret, void *args [], void *userdata) {
    static_cast<TCallbackData *> (userdata)->handleCallback (ret, args);
}


TExternalRoutine::TExternalRoutine (const std::string &libName, const std::string &routineName, bool useFFI):
  libName (libName), routineName (routineName), useFFI (useFFI), libHandle (nullptr), func (nullptr), parameterOffset (0), hasCallback (false), useTempResult (false) {
    resultType = &ffi_type_void;
}

TExternalRoutine::~TExternalRoutine () {
    if (libHandle)
        dlclose (libHandle);
}

void TExternalRoutine::addParameter (ssize_t offset, TExternalType type) {
    if (type == TExternalType::ExtSet && !setTypeBuilt)
        buildSetType ();
    offsets.push_back (offset);
    types.push_back (ffiTypes [static_cast<std::size_t> (type)]);
    callbackDescriptions.push_back (TCallbackDescription ());
}

void TExternalRoutine::addCallback (ssize_t offset, const TCallbackDescription &callbackDescription) {
    offsets.push_back (offset);
    types.push_back (&ffi_type_pointer);
    callbackDescriptions.push_back (callbackDescription);
    hasCallback = true;
}

void TExternalRoutine::setResult (ssize_t offset, TExternalType type) {
    static const std::map<TExternalType, std::size_t> intTypeSizes = {{TExternalType::ExtInt8, 1}, {TExternalType::ExtInt16, 2}, {TExternalType::ExtInt32, 4}, {TExternalType::ExtInt64, 8}};
    if (type == TExternalType::ExtSet && !setTypeBuilt)
        buildSetType ();
    resultOffset = offset;
    resultType = ffiTypes [static_cast<std::size_t> (type)];
    std::map<TExternalType, std::size_t>::const_iterator it = intTypeSizes.find (type);
    useTempResult = it != intTypeSizes.end () && it->second < sizeof (ffi_arg);
    if (useTempResult)
        tempResultSize = it->second;
}

void TExternalRoutine::setParameterOffset (ssize_t n) {
    parameterOffset = n;
}

void TExternalRoutine::callFFI (void *parameterStart, TPCodeInterpreter &pcodeInterpreter) {
    const std::size_t n = offsets.size ();    
    void **values = reinterpret_cast<void **> (alloca (n * sizeof (void *)));
    
    TStackMemory *const base = static_cast<TStackMemory *> (parameterStart);
    for (std::size_t i = 0; i < n; ++i)
        values [i] = base + offsets [i];
    if (hasCallback)
        for (std::size_t i = 0; i < n; ++i)
            if (callbackDescriptions [i].isValid)
                values [i] = pcodeInterpreter.getPCodeMemory ().getCallbackData (*static_cast<std::size_t *> (values [i]), callbackDescriptions [i]).getFuncPtrPtr ();
    if (resultType != &ffi_type_void && !useTempResult)
        ffi_call (&cif, func, *static_cast<void **> (static_cast<void *> (base + resultOffset)), values);
    else {
        ffi_arg result;
        ffi_call (&cif, func, &result, values);
        if (useTempResult)
            // TODO: big endian
            memcpy (*static_cast<void **> (static_cast<void *> (base + resultOffset)), &result, tempResultSize);
    }
}

bool TExternalRoutine::load () {
    if (libName.empty ())
        libHandle = dlopen (nullptr, RTLD_NOW);
    else
        libHandle = dlopen (libName.c_str (), RTLD_NOW);
        
    if (!libHandle)  {
        perror ("");
        throw TRuntimeError (TRuntimeResult::TCode::FFIError, "Cannot open " + libName);
    }
    func = reinterpret_cast<void (*) ()> (dlsym (libHandle, routineName.c_str ()));
    if (!func) 
        throw TRuntimeError (TRuntimeResult::TCode::FFIError, "Cannot resolve " + routineName + " in " + libName);
    if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, types.size (), resultType, types.empty () ? nullptr : &types [0]) != FFI_OK)
        throw TRuntimeError (TRuntimeResult::TCode::FFIError, "FFI failed for "  + routineName + " in " + libName);
//    values.resize (offsets.size ());
    return true;
}

}
