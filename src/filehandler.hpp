/** \file filehandler.hpp
*/

#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <cstdint>

namespace statpascal {

class TFileHandler {
public:
    TFileHandler () = default;
    virtual ~TFileHandler () = default;
    
    virtual void reset () = 0;
    virtual void rewrite () = 0;
    virtual void append () = 0;
    
    virtual void flush () = 0;
    virtual bool eof () = 0;
};

class TBinaryFileHandler final: public TFileHandler {
using inherited = TFileHandler;
public:
    TBinaryFileHandler (const std::string &fn);
    virtual ~TBinaryFileHandler () = default;
        
    std::size_t write (const void *, std::size_t size);
    std::size_t read (void *, std::size_t size);
    
    void seek (std::int64_t pos);
    std::int64_t filepos ();
    
    virtual void reset () override;
    virtual void rewrite () override;
    virtual void append () override;

    virtual void flush () override;    
    virtual bool eof () override;
    
private:
    const std::string fn;
    std::fstream f;
};

class TTextFileBaseHandler: public TFileHandler {
using inherited = TFileHandler;
public:
    TTextFileBaseHandler ();
    virtual ~TTextFileBaseHandler () = default;

    virtual void write (const std::string &) = 0;
    virtual std::ostream & getOutputStream () = 0;
    
    double getDouble ();
    std::int64_t getInteger ();
    unsigned char getChar ();
    std::string getString ();
    void skipLine ();

    virtual void flush () override;    
    virtual bool eof () override;
    bool eoln ();

private:
    virtual std::string readLine () = 0;
    virtual bool streamEof () = 0;
    
    template<typename T> T getValue ();
    bool fillInputBuffer ();

    bool initialFill;    
    std::string inputBuffer;
    std::istringstream inputStream;
};    

class TTextFileHandler: public TTextFileBaseHandler {
using inherited = TTextFileBaseHandler;
public:
    TTextFileHandler (const std::string &fn);
    virtual ~TTextFileHandler () = default;
    
    virtual void write (const std::string &) override;
    virtual std::ostream & getOutputStream () override;
    
    virtual void reset () override;
    virtual void rewrite () override;
    virtual void append () override;
    
private:
    virtual std::string readLine () override;
    virtual bool streamEof () override;

    const std::string fn;    
    std::fstream f;
};

class TStdioHandler: public TTextFileBaseHandler {
using inherited = TTextFileBaseHandler;
public:
    TStdioHandler () = default;
    virtual ~TStdioHandler () = default;

    virtual void write (const std::string &) override;
    virtual std::ostream & getOutputStream () override;
    
    virtual void reset () override;
    virtual void rewrite () override;
    virtual void append () override;
    
private:
    virtual std::string readLine () override;
    virtual bool streamEof () override;
};

}
