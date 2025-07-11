/** \file compiler.hpp
*/

#pragma once

#include <string>
#include <memory>
#include <vector>

namespace statpascal {

class TCompilerImpl;
class TCodeGenerator;

struct TCompilerImplDeleter {
    void operator () (TCompilerImpl *);
};

class TCompiler {
public:
    explicit TCompiler (TCodeGenerator &);
    ~TCompiler ();
    
    void setSource (const std::string &);
    void setSource (std::string &&);
    void setFilename (const std::string &);
    void setUnitSearchPathes (const std::vector<std::string> &);

    enum TCompileResult {Error, UnitCompiled, ProgramCompiled};    
    TCompileResult compile ();
    
private:
    std::unique_ptr<TCompilerImpl, TCompilerImplDeleter> pImpl;
    
    const TCompilerImpl *impl () const;
    TCompilerImpl *impl ();
};

}
