/** \file predefined.hpp
*/

#pragma once

#include "expression.hpp"

namespace statpascal {

class TRuntimeRoutine: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TRuntimeRoutine (TType *returnType);
     
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    std::vector<TSyntaxTreeNode *> getTransformedNodes ();
   
    struct TFormatArguments {
        TExpressionBase *expression, *length, *precision;
    };
protected:
    void appendTransformedNode (TSyntaxTreeNode *);
    void checkFormatArguments (TFormatArguments &argument, const TType *outputtype, TBlock &);
    
private:
    std::vector<TSyntaxTreeNode *> transformedNodes;
};


class TCombineRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TCombineRoutine (TBlock &, std::vector<TExpressionBase *> &&);
    
private:
    TFunctionCall *createCombineCall (TBlock &, TType *resultType, std::vector<TExpressionBase *> &&args);
};


class TResizeRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TResizeRoutine (TBlock &, std::vector<TExpressionBase *> &&);
};


class TPredefinedRoutine: public TExpressionBase {
using inherited = TExpressionBase;
public:
    enum TRoutine {
        Chr, Addr, Ord, Odd, Succ, Pred, Inc, Dec,
        New, Dispose,
        Exit, Break, Halt
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
