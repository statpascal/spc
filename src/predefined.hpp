/** \file predefined.hpp
*/

#pragma once

#include "expression.hpp"

namespace statpascal {

class TPredefinedRoutine: public TExpressionBase {
using inherited = TExpressionBase;
public:
    enum TRoutine {
        Odd, Succ, Pred, Inc, Dec, Exit
    };

    TPredefinedRoutine (TRoutine, TType *returnType, std::vector<TExpressionBase *> &&args);
    static TExpressionBase *parse (const std::string &identifier, TBlock &);
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TRoutine getRoutine () const;
    const std::vector<TExpressionBase *> &getArguments () const;

private:
    TRoutine routine;
    std::vector<TExpressionBase *> arguments;
};

}
