#include "anymanager.hpp"
#include "anyvalue.hpp"
#include "vectordata.hpp"

namespace statpascal {

void TAnySingleValueManager::init (void *base) {
    memset (base, 0, sizeof (void *));
}

void TAnySingleValueManager::destroy (void *base) {
    static_cast<TAnyValue *> (base)->~TAnyValue ();
}

void TAnySingleValueManager::copy (const void *source, void *dest) {
    new (dest) TAnyValue (*static_cast<const TAnyValue *> (source));
}


TAnyArrayManager::TAnyArrayManager (TAnyManager *anyType, std::size_t count, std::size_t size):
  anyType (anyType), count (count), size (size) {
}

TAnyArrayManager::~TAnyArrayManager () {
    delete anyType;
}

void TAnyArrayManager::init (void *base) {
    for (std::size_t i = 0; i < count; ++i)
        anyType->init (static_cast<unsigned char*> (base) + i * size);
}

void TAnyArrayManager::destroy (void *base) {
    for (std::size_t i = 0; i < count; ++i)
        anyType->destroy (static_cast<unsigned char*> (base) + i * size);
}

void TAnyArrayManager::copy (const void *source, void *dest) {
    for (std::size_t i = 0; i < count; ++i)
        anyType->copy (static_cast<const unsigned char*> (source) + i * size, static_cast<unsigned char*> (dest) + i * size);
}

TAnyRecordManager::~TAnyRecordManager () {
    for (TComponent &c: components)
        delete c.anyType;
}

void TAnyRecordManager::appendComponent (TAnyManager *anyType, ssize_t offset) {
    components.push_back ({anyType, offset});
}

bool TAnyRecordManager::isEmpty () {
    return components.empty ();
}

void TAnyRecordManager::init (void * base) {
    for (TComponent &c: components)
        c.anyType->init (static_cast<unsigned char*> (base) + c.offset);
}

void TAnyRecordManager::destroy (void * base) {
    for (TComponent &c: components)
        c.anyType->destroy (static_cast<unsigned char*> (base) + c.offset);
}

void TAnyRecordManager::copy (const void *source, void *dest) {
    for (TComponent &c: components)
        c.anyType->copy (static_cast<const unsigned char*> (source) + c.offset, static_cast<unsigned char*> (dest) + c.offset);
}

}
