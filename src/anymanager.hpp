/** }file anymanager.hpp
*/

#pragma once

#include <vector>
#include <string>	// where is ssize_t !!!! 

namespace statpascal {

class TAnyManager {
public:
    virtual ~TAnyManager () = default;
    virtual void init (void *base) = 0;
    virtual void destroy (void *base) = 0;
    virtual void copy (const void *source, void *dest) = 0;
};

/*

class TAnyCopyManager: public TAnyManager {
public:
    TAnyCopyManager (std::size_t size);

    virtual void destroy (void *base) override;
    virtual void copy (const void *source, void *dest) override;
    
private:
    std::size_t size;
};

*/

class TAnySimpleValueManager: public TAnyManager {
public:
    virtual void init (void *base) override;
    virtual void destroy (void *base) override;
    virtual void copy (const void *source, void *dest) override;
};

class TAnyVectorManager: public TAnyManager {
public:
    virtual void init (void *base) override;
    virtual void destroy (void *base) override;
    virtual void copy (const void *source, void *dest) override;
};

class TAnyArrayManager: public TAnyManager {
public:
    TAnyArrayManager (TAnyManager *anyType, std::size_t count, std::size_t size);
    virtual ~TAnyArrayManager ();    
    
    virtual void init (void *base) override;
    virtual void destroy (void *base) override;
    virtual void copy (const void *source, void *dest) override;    
    
private:
    TAnyManager *anyType;
    std::size_t count, size;
};

class TAnyRecordManager: public TAnyManager {
public:
    TAnyRecordManager () = default;
    virtual ~TAnyRecordManager ();
    void appendComponent (TAnyManager *anyType, ssize_t offset);
    bool isEmpty ();    
    
    virtual void init (void *base) override;
    virtual void destroy (void *base) override;
    virtual void copy (const void *source, void *dest) override;    
    
private:
    struct TComponent {
        TAnyManager *anyType;
        ssize_t offset;
    };
    std::vector<TComponent> components;
};

}
