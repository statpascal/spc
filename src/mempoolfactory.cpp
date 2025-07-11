#include "mempoolfactory.hpp"

#include <algorithm>
#include <vector>

namespace statpascal {

class TMemoryPoolFactory::TMemoryPoolFactoryImpl {
public:
    TMemoryPoolFactoryImpl (std::size_t chunkSize_para, std::size_t align);
    ~TMemoryPoolFactoryImpl ();
    
    void *allocate (std::size_t);
    
private:    
    std::size_t calcAlign (std::size_t size);
    void allocateNewChunk ();
    
    std::vector<char *> oldChunks;
    std::size_t currentChunkPosition;
    const std::size_t align, chunkSize, sizeFieldSize;
    char *currentChunk;
};

void TMemoryPoolFactory::TMemoryPoolFactoryImplDeleter::operator () (TMemoryPoolFactoryImpl *p) {
    delete p;
}

TMemoryPoolFactory::TMemoryPoolFactoryImpl::TMemoryPoolFactoryImpl (std::size_t chunkSize_para, std::size_t align): 
  align (align), 
  chunkSize (calcAlign (chunkSize_para)),
  sizeFieldSize (calcAlign (sizeof (std::size_t))),
  currentChunk (nullptr) {
    allocateNewChunk ();
}

TMemoryPoolFactory::TMemoryPoolFactoryImpl::~TMemoryPoolFactoryImpl () {
    oldChunks.push_back (currentChunk);
    for (char *pool: oldChunks) {
        char *poolptr = pool;
        std::size_t size = *reinterpret_cast<std::size_t *> (poolptr);
        while (size) {
            reinterpret_cast<TMemoryPoolObject *> (poolptr + calcAlign (sizeof (std::size_t)))->~TMemoryPoolObject ();
            poolptr += size;
            size = *reinterpret_cast<std::size_t *> (poolptr);
        }
        operator delete (pool);
    }
}

void *TMemoryPoolFactory::TMemoryPoolFactoryImpl::allocate (std::size_t size) {
    std::size_t required = sizeFieldSize + calcAlign (size);
    if (required > chunkSize - sizeFieldSize)
        abort ();
    if (currentChunkPosition + required > chunkSize - sizeFieldSize)
        allocateNewChunk ();
    
    char *result = currentChunk + currentChunkPosition;
    *reinterpret_cast<std::size_t *> (result) = required;
    currentChunkPosition += required;
    return result + sizeFieldSize;
}

std::size_t TMemoryPoolFactory::TMemoryPoolFactoryImpl::calcAlign (std::size_t size) {
    return (size + align - 1) & ~(align - 1);
}

void TMemoryPoolFactory::TMemoryPoolFactoryImpl::allocateNewChunk () {
    if (currentChunk)
        oldChunks.push_back (currentChunk);
    currentChunk = reinterpret_cast<char *> (operator new (chunkSize));
    std::fill (currentChunk, currentChunk + chunkSize, 0);
    currentChunkPosition = 0;
}


TMemoryPoolFactory::TMemoryPoolFactory (std::size_t chunkSize, std::size_t align):
  pImpl (new TMemoryPoolFactoryImpl (chunkSize, align)) {
}

void *TMemoryPoolFactory::allocate (std::size_t size) {
    return pImpl->allocate (size);
}

}

#ifdef TEST

#include <iostream>

class C1: public statpascal::TMemoryPoolObject {
public:
    C1 (int a, int b) { std::cout << a << ' ' << b << std::endl; }
protected:
    virtual ~C1 () { std::cout << "~C1" << std::endl; }
private:
    int a, b;
};

class C2: public statpascal::TMemoryPoolObject {
public:
    C2 (const std::string &s) { std::cout << s << std::endl; }
protected:
    virtual ~C2 () { std::cout << "~C2" << std::endl; }
private:    
    std::string s;
};

class C3: public statpascal::TMemoryPoolObject {
public:
    C3 (double v) { std::cout << v << std::endl; }
protected:
    virtual ~C3 () { std::cout << "~C3" << std::endl; }
private:
    double v;
};


int main () {
    statpascal::TMemoryPoolFactory factory (1024);
    for (int i = 0; i < 10000; ++i) {
        factory.create<C1> (1, 2);
        factory.create<C2> ("Hello, world");
        factory.create<C3> (3.1415);
    }
}


#endif
