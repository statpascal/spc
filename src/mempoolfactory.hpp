/** \file mempoolfactory.hpp
*/

#pragma once

#include <memory>

namespace statpascal {

class TMemoryPoolObject {
protected:
    virtual ~TMemoryPoolObject () {}
    friend class TMemoryPoolFactory;
};


class TMemoryPoolFactory final {
public:
    TMemoryPoolFactory (std::size_t chunkSize, std::size_t align = sizeof (std::size_t));
    ~TMemoryPoolFactory () = default;
    
    TMemoryPoolFactory (const TMemoryPoolFactory &) = delete;
    TMemoryPoolFactory &operator = (const TMemoryPoolFactory &) = delete;

    template<typename T, typename ...Args> T *create (Args&&... args);

private:
    void *allocate (std::size_t);

    class TMemoryPoolFactoryImpl;
    struct TMemoryPoolFactoryImplDeleter {
        void operator () (TMemoryPoolFactoryImpl *);
    };
    std::unique_ptr<TMemoryPoolFactoryImpl, TMemoryPoolFactoryImplDeleter> pImpl;
};

template<typename T, typename ...Args> inline T *TMemoryPoolFactory::create (Args&&... args) {
    TMemoryPoolObject *result = new (allocate (sizeof (T))) T (std::forward<Args> (args)...);
    return static_cast<T *> (result);
}

}
