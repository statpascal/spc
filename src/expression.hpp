/** \file expression.hpp
*/

#pragma once

#include <vector>

#include "compilerimpl.hpp"

namespace statpascal {

class TCodeGenerator; class TFunctionCall;

class TExpressionBase: public TSyntaxTreeNode {
using inherited = TSyntaxTreeNode;
public:
    TExpressionBase ();
    
    virtual bool isLValue () const;
    virtual bool isLValueDereference () const;
    virtual bool isConstant () const;
    virtual bool isSymbol () const;
    virtual bool isReference () const;
    virtual bool isRoutine () const;
    virtual bool isFunctionCall () const;
    virtual bool isTypeCast () const;
    virtual bool isVectorIndex () const;
    
    TType *getType () const;
    void setType (TType *);
    
    static bool checkTypeConversion (TType *required, TExpressionBase *&expr, TBlock &);
    static bool performTypeConversion (TType *required, TExpressionBase *&expr, TBlock &);
    static TType *convertBaseType (TExpressionBase *&expression, TBlock &);
    
    static TExpressionBase *createVariableAccess (const std::string &name, TBlock &);
    static TFunctionCall *createRuntimeCall (const std::string &name, TType *result, std::vector<TExpressionBase *> &&args, TBlock &, bool checkParameter);
    static TExpressionBase *createInt64Constant (std::int64_t val, TBlock &block);
    template<typename T> static TExpressionBase *createConstant (T val, TType *type, TBlock &);
    
protected:
    static TType *checkOperatorTypes (TExpressionBase *&left, TExpressionBase *&right, TToken operation, TBlock &);
    
    static TType *checkSetOperatorTypes (TExpressionBase *&left, TExpressionBase *&right, TToken operation, TBlock &);
    static TType *checkVectorOperatorTypes (TExpressionBase *&left, TExpressionBase *&right, TToken operation, TBlock &);
    
    static TType *getVectorBaseType (TExpressionBase *expression, TBlock &);
    static TType *retrieveVectorAndBaseType (TExpressionBase *&expr, TBlock &);
    static bool checkTypeConversionVector (TType *required, TExpressionBase *&expression, TBlock &);
    
    /** create call for functions without parameters */
    static bool createFunctionCall (TExpressionBase *&base, TBlock &, bool recursive);
    
    
    /** merge constant expression */
    static bool mergeConstants (TExpressionBase *&left, TExpressionBase *right, TType *type, TToken operation, TBlock &);
    
    /** evaluate unary operator (-, not) for constant expression */
    static bool evaluateConstant (TExpressionBase *&expr, TType *type, TToken operation, TBlock &);

private:    
    TType *type;
};


class TTypeCast: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TTypeCast (TType *required, TExpressionBase *base);
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TType *getRequiredType () const;
    TExpressionBase *getExpression () const;
    
    virtual bool isTypeCast () const override;
    
private:
    // muess behandeln: subrange -> basetype, basetype->subrange mit Check

    TType *required;
    TExpressionBase *base;
};

inline TType *TTypeCast::getRequiredType () const {
    return required;
}

inline TExpressionBase *TTypeCast::getExpression () const {
    return base;
}

class TExpression final: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TExpression (TExpressionBase *left, TExpressionBase *right, TToken operation, TType *type);
    
    static TExpressionBase *parse (TBlock &);
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getLeftExpression () const;
    TExpressionBase *getRightExpression () const;
    TToken getOperation () const;
    
private:
    TExpressionBase *left, *right;
    TToken operation;
};

inline TExpressionBase *TExpression::getLeftExpression () const {
    return left;
}

inline TExpressionBase *TExpression::getRightExpression () const {
    return right;
}

inline TToken TExpression::getOperation () const {
    return operation;
}


class TSimpleExpression: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TSimpleExpression (TExpressionBase *left, TExpressionBase *right, TToken operation, TType *type);
    
    static TExpressionBase *parse (TBlock &);
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getLeftExpression () const;
    TExpressionBase *getRightExpression () const;
    TToken getOperation () const;
    
private:
    TExpressionBase *left, *right;
    TToken operation;
};

inline TExpressionBase *TSimpleExpression::getLeftExpression () const {
    return left;
}

inline TExpressionBase *TSimpleExpression::getRightExpression () const {
    return right;
}

inline TToken TSimpleExpression::getOperation () const {
    return operation;
}


class TTerm: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TTerm (TExpressionBase *left, TExpressionBase *right, TToken operation, TType *type);
    
    static TExpressionBase *parse (TBlock &);
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getLeftExpression () const;
    TExpressionBase *getRightExpression () const;
    TToken getOperation () const;
    
private:
    TExpressionBase *left, *right;
    TToken operation;
};

inline TExpressionBase *TTerm::getLeftExpression () const {
    return left;
}

inline TExpressionBase *TTerm::getRightExpression () const {
    return right;
}

inline TToken TTerm::getOperation () const {
    return operation;
}


class TFactor: public TExpressionBase {
using inherited = TExpressionBase;
public:
    static TExpressionBase *parse (TBlock &);
    
private:
    static TExpressionBase *parseIdentifier (TBlock &);
    static TExpressionBase *parseFunctionCall (TExpressionBase *function, TBlock &);
    static TExpressionBase *parseTypeConversion (TSymbol *, TBlock &);
    
    static TExpressionBase *parseIndex (TExpressionBase *base, TBlock &);
    static TExpressionBase *parseRecordComponent (TExpressionBase *base, TBlock &);
    static TExpressionBase *parsePointerDereference (TExpressionBase *base, TBlock &);
    static TExpressionBase *parseSetExpression (TBlock &);
    static TExpressionBase *parseAddressOperator (TBlock &);
};

class TPrefixedExpression: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TPrefixedExpression (TExpressionBase *base, TToken operation, TBlock &);
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getExpression () const;
    TToken getOperation () const;
    
private:
    TExpressionBase *base;    
    TToken operation;
};

inline TExpressionBase *TPrefixedExpression::getExpression () const {
    return base;
}

inline TToken TPrefixedExpression::getOperation () const {
    return operation;
}

// Specializations of factor

class TFunctionCall: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TFunctionCall (TExpressionBase *function, std::vector<TExpressionBase *> &&args, TBlock &, bool checkParameterTypes);
    
    virtual bool isFunctionCall () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    void ignoreReturnValue (bool);
    bool isIgnoreReturn () const;
    TExpressionBase *getFunction () const;
    const std::vector<TExpressionBase *> &getArguments () const;
    
//    const TSymbol *getFunctionReturnTempStorage () const;
    TExpressionBase *getReturnStorage () const;		// L-Value
//    TExpressionBase *getReturnTempStorage () const;
    void setReturnStorage (TExpressionBase *returnStorage, TBlock &);	// L-value
    
private:
    TExpressionBase *function;
    std::vector<TExpressionBase *> args;
    bool ignoreReturn;
    TSymbol *returnSymbol;
    TExpressionBase *returnStorage;
};

inline TExpressionBase *TFunctionCall::getFunction () const {
    return function;
}

inline const std::vector<TExpressionBase *> &TFunctionCall::getArguments () const {
    return args;
}


class TConstantValue: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TConstantValue (const TSimpleConstant *);
    TConstantValue (TSymbol *);
    
    virtual bool isConstant () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    const TSimpleConstant *getConstant () const;
    TSymbol *getSymbol () const;

private:
    const TSimpleConstant *constant;
    TSymbol *symbol;
};

inline const TSimpleConstant *TConstantValue::getConstant () const {
    return symbol ? dynamic_cast<const TSimpleConstant *> (symbol->getConstant ()) : constant;
}

inline TSymbol *TConstantValue::getSymbol () const {
    return symbol;
}


class TRoutineValue: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TRoutineValue (const std::string &identifier, TBlock &);
    
    virtual bool isRoutine () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;

    void resolveCall (std::vector<TExpressionBase *> args, TBlock &);
    bool resolveConversion (const TRoutineType *required);

    TSymbol *getSymbol () const;
    
private:
    void resolveOverload (TSymbol *);

    TSymbol *symbol;	// resolved overload
    std::vector<TSymbol *> symbolOverloads;
};

inline TSymbol *TRoutineValue::getSymbol () const {
    return symbol;
}


class TVariable: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TVariable (TSymbol *, TBlock &);
    
    virtual bool isLValue () const override;
    virtual bool isSymbol () const override;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TSymbol *getSymbol ();

private:
    TSymbol *symbol;
};

inline TSymbol *TVariable::getSymbol () {
    return symbol;
}


class TReferenceVariable: public TVariable {
using inherited = TVariable;
public:
    TReferenceVariable (TSymbol *, TBlock &);

    virtual bool isReference () const override;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
};


class TLValueDereference: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TLValueDereference (TExpressionBase *base);
    
    TExpressionBase *getLValue () const;
    
    virtual bool isLValueDereference () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
private:
    TExpressionBase *base;
};


class TArrayIndex: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TArrayIndex (TExpressionBase *base, TExpressionBase *index, TType *type);
    
    virtual bool isLValue () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getBaseExpression () const;
    TExpressionBase *getIndexExpression () const;
    
private:
    TExpressionBase *base, *index;
};

inline TExpressionBase *TArrayIndex::getBaseExpression () const {
    return base;
}
 
inline TExpressionBase *TArrayIndex::getIndexExpression () const {
    return index;
}


class TRecordComponent: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TRecordComponent (TExpressionBase *base, const std::string &component, const TSymbol *);
    
    virtual bool isLValue () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getExpression () const;
    const std::string getComponent () const;
    
private:
    TExpressionBase *base;
    const std::string component;
};

inline TExpressionBase *TRecordComponent::getExpression () const {
    return base;
}

inline const std::string TRecordComponent::getComponent () const {
    return component;
}


class TPointerDereference: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TPointerDereference (TExpressionBase *base);
    
    virtual bool isLValue () const override;
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getExpression () const;
    
private:
    TExpressionBase *expr;
};

inline TExpressionBase *TPointerDereference::getExpression () const {
    return expr;
}

class TVectorIndex: public TExpressionBase {
using inherited = TExpressionBase;
public:
    enum class TIndexKind {
        IntVec, BoolVec, Int
    };
    
    TVectorIndex (TExpressionBase *base, TExpressionBase *index, TType *resultType, TIndexKind indexKind, TBlock &block);
    
    virtual bool isLValue () const override;
    virtual bool isVectorIndex () const override;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
private:
    TFunctionCall *runtimeCall;
    bool lValue;
};

}