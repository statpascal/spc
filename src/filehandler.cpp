#include "filehandler.hpp"

#include <iostream>

namespace statpascal {

TBinaryFileHandler::TBinaryFileHandler (const std::string &fn):
  fn (fn) {
}

std::size_t TBinaryFileHandler::write (const void *buf, std::size_t size) {
    std::streampos p = f.tellp ();
    f.write (reinterpret_cast<const char *> (buf), size);
    return f.tellp () - p;
}

std::size_t TBinaryFileHandler::read (void *buf, std::size_t size) {
    f.read (reinterpret_cast<char *> (buf), size);
    return f.gcount ();
}

void TBinaryFileHandler::seek (std::int64_t pos) {
    f.seekg (pos);
}

std::int64_t TBinaryFileHandler::filepos () {
    return f.tellp ();
}

void TBinaryFileHandler::reset () {
    f.open (fn, std::ios::in | std::ios::out | std::ios::binary);
}

void TBinaryFileHandler::rewrite () {
    f.open (fn, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
}

void TBinaryFileHandler::append () {
    f.open (fn, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
}

void TBinaryFileHandler::flush () {
    f.flush ();
}

bool TBinaryFileHandler::eof () {
    return f.eof () || f.peek () == EOF;
}

//

TTextFileBaseHandler::TTextFileBaseHandler ():
  initialFill (true) {
}

template<typename T> T TTextFileBaseHandler::getValue () {
    T val = T ();
    if (initialFill)
        fillInputBuffer ();
    while (!(inputStream >> val) && !eof ())
        fillInputBuffer ();
    return val;
}

double TTextFileBaseHandler::getDouble () {
    return getValue<double> ();
}

std::int64_t TTextFileBaseHandler::getInteger () {
    return getValue<std::int64_t> ();
}

unsigned char TTextFileBaseHandler::getChar () {
    char c = 0;
    if (!inputStream.get (c)) {
        if (fillInputBuffer ())
            inputStream.get (c);
        else
            ; // ERROR
    }
    return static_cast<unsigned char> (c);
}

std::string TTextFileBaseHandler::getString () {
    std::string s;
    char c;
    
    if (initialFill)
        fillInputBuffer ();
    
    while (inputStream.peek () != '\n' && inputStream.get (c))
        s += c;
    return s;
}

void TTextFileBaseHandler::skipLine () {
    fillInputBuffer ();
}

void TTextFileBaseHandler::flush () {
    getOutputStream ().flush ();
}

bool TTextFileBaseHandler::eof () {
    if (initialFill)
        fillInputBuffer ();
    return inputStream.peek () == EOF && streamEof ();
}

bool TTextFileBaseHandler::eoln () {
    return eof () || inputStream.peek () == '\n';
}

bool TTextFileBaseHandler::fillInputBuffer () {
    initialFill = false;
    
    std::string s = readLine ();
        
    inputStream.clear ();
    inputStream.str (s);
    return !s.empty ();
}

// ---

TTextFileHandler::TTextFileHandler (const std::string &fn):
  fn (fn) {
}

void TTextFileHandler::write (const std::string &s) {
    f << s;
}

std::ostream &TTextFileHandler::getOutputStream () {
    return f;
}

void TTextFileHandler::reset () {
    f.open (fn, std::ios::in | std::ios::out);
}

void TTextFileHandler::rewrite () {
    f.open (fn, std::ios::in | std::ios::out | std::ios::trunc);
}

void TTextFileHandler::append () {
    f.open (fn, std::ios::in | std::ios::out | std::ios::ate);
}

std::string TTextFileHandler::readLine () {
    std::string s;
    std::getline (f, s);
    if (!f.eof ())
        s += '\n';
    return s;
}

bool TTextFileHandler::streamEof () {
    return f.peek () == EOF || f.eof ();
}

// ---

void TStdioHandler::write (const std::string &s) {
    std::cout << s;
}

std::ostream &TStdioHandler::getOutputStream () {
    return std::cout;
}

void TStdioHandler::reset () {
}

void TStdioHandler::rewrite () {
}

void TStdioHandler::append () {
}

std::string TStdioHandler::readLine () {
    std::string s;
    getline (std::cin, s);
    if (!std::cin.eof ())
        s += '\n';
    return s;
}

bool TStdioHandler::streamEof () {
    return std::cin.peek () == EOF || std::cin.eof ();
}

}