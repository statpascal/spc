/** \file statements.hpp
*/

#pragma once

#include <vector>


#include "constant.hpp"
#include "syntaxtreenode.hpp"
#include "lexer.hpp"

namespace statpascal {

class TBlock;
class TExpressionBase;
class TSymbol;
class TType;
class TCodeGenerator;

class TStatement: public TSyntaxTreeNode {
public:
    TStatement () = default;
    virtual ~TStatement () = default;
    
    static TStatement *parse (TBlock &);
    
protected:    
    static TSymbol *createLocalLabel (TBlock &);
    static TSymbol *createCaseLabel (TBlock &);
    static std::vector<TStatement *> parseStatementSequence (TBlock &declarations, TToken endToken);
};

class TEmptyStatement: public TStatement {
public:
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
};

class TLabeledStatement: public TStatement {
using inherited = TStatement;
public:
    TLabeledStatement (TSymbol *label, TBlock &);
    TLabeledStatement (TSymbol *label, TStatement *statement);
    virtual ~TLabeledStatement () = default;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TSymbol *getLabel () const;
    TStatement *getStatement () const;
    
private:
    void parse (TBlock &);

    TSymbol *label;
    TStatement *statement;
};

inline TSymbol *TLabeledStatement::getLabel () const {
    return label;
}

inline TStatement *TLabeledStatement::getStatement () const {
    return statement;
}


class TAssignment: public TStatement {
using inherited = TStatement;
public:
    TAssignment (TExpressionBase *lValue, TExpressionBase *expr);
    virtual ~TAssignment () = default;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getLValue () const;
    TExpressionBase *getExpression () const;
    
private:
    TExpressionBase *lValue, *expr;
};

inline TAssignment::TAssignment (TExpressionBase *lValue, TExpressionBase *expr):
  lValue (lValue), expr (expr) {
}

inline TExpressionBase *TAssignment::getLValue () const {
    return lValue;
}

inline TExpressionBase *TAssignment::getExpression () const {
    return expr;
}


class TRoutineCall: public TStatement {
using inherited = TStatement;
public:
    explicit TRoutineCall (TExpressionBase *routineCall);
    virtual ~TRoutineCall () = default;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;

    TExpressionBase *getRoutineCall () const;
    
private:
    TExpressionBase *routineCall;
};

inline TRoutineCall::TRoutineCall (TExpressionBase *routineCall):
  routineCall (routineCall) {
}

inline TExpressionBase *TRoutineCall::getRoutineCall () const {
    return routineCall;
}


class TSimpleStatement: public TStatement {
public:
    static TStatement *parse (TBlock &declarations);
};


/*
class TSimpleStatement: public TStatement {
using inherited = TStatement;
public:
    explicit TSimpleStatement (TBlock &);
    TSimpleStatement (TExpressionBase *left, TExpressionBase *right);
    virtual ~TSimpleStatement () = default;

    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getLeftExpression () const;
    TExpressionBase *getRightExpression () const;
private:    
    void parse (TBlock &);
    
    TExpressionBase *left, *right;
};

inline TExpressionBase *TSimpleStatement::getLeftExpression () const {
    return left;
}

inline TExpressionBase *TSimpleStatement::getRightExpression () const {
    return right;
}
*/

class TIfStatement: public TStatement {
using inherited = TStatement;
public:
    explicit TIfStatement (TBlock &);
    TIfStatement (TExpressionBase *condition, TStatement *statement);
    virtual ~TIfStatement () = default;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TExpressionBase *getCondition () const;
    TStatement *getStatement1 () const;
    TStatement *getStatement2 () const;
    
private:    
    void parse (TBlock &);
    
    TExpressionBase *condition;
    TStatement *statement1, *statement2;
};

inline TExpressionBase *TIfStatement::getCondition () const {
    return condition;
}

inline TStatement *TIfStatement::getStatement1 () const {
    return statement1;
}

inline TStatement *TIfStatement::getStatement2 () const {
    return statement2;
}


class TStatementSequence: public TStatement {
using inherited = TStatement;
public:
    explicit TStatementSequence (TBlock &);
    explicit TStatementSequence (std::vector<TStatement *> &&);
    virtual ~TStatementSequence () = default;

    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    const std::vector<TStatement *> &getStatements () const;
    
    void appendFront (TStatement *);
    void appendBack (TStatement *);
    
private:    
    void parse (TBlock &);
    
    std::vector<TStatement *> statements;
};

inline const std::vector<TStatement *> &TStatementSequence::getStatements () const {
    return statements;
}


class TRepeatStatement: public TStatement {
public:
    static TStatement *parse (TBlock &declarations);
};


class TWhileStatement: public TStatement {
public:
    static TStatement *parse (TBlock &declarations);
};


class TCaseStatement: public TStatement {
using inherited = TStatement;
public:
    explicit TCaseStatement (TBlock &);
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;

    using TLabel        = struct { std::int64_t a, b; };    
    using TLabelList    = std::vector<TLabel>;
    using TCase         = struct { TLabelList labels; TStatement *statement; TSymbol *jumpLabel; };
    using TCaseList     = std::vector<TCase>;
    
    using TSortedEntry  = struct { TLabel label; TSymbol *jumpLabel; };
    using TSortedLabels = std::vector<TSortedEntry>;
    
    TExpressionBase *getExpression ();
    const TCaseList &getCaseList ();
    const TSortedLabels &getSortedLabels ();
    TStatement *getDefaultStatement ();
    
    std::int64_t getMinLabel () const;
    std::int64_t getMaxLabel () const;
    
private:
    void parse (TBlock &);

    TExpressionBase *expression;
    TCaseList caseList;
    TStatement *defaultStatement;
    std::int64_t minLabel, maxLabel;
    TSortedLabels sortedLabels;
};

inline TExpressionBase *TCaseStatement::getExpression () {
    return expression;
}

inline const TCaseStatement::TCaseList &TCaseStatement::getCaseList () {
    return caseList;
}

inline const TCaseStatement::TSortedLabels &TCaseStatement::getSortedLabels () {
    return sortedLabels;
}

inline TStatement *TCaseStatement::getDefaultStatement () {
    return defaultStatement;
}

inline std::int64_t TCaseStatement::getMinLabel () const {
    return minLabel;
}

inline std::int64_t TCaseStatement::getMaxLabel () const {
    return maxLabel;
}


class TForStatement: public TStatement {
public:
    static TStatement *parse (TBlock &);
};


class TGotoStatement: public TStatement {
using inherited = TStatement;
public:
    TGotoStatement (TBlock &);
    TGotoStatement (TSymbol *label, TExpressionBase *condition = nullptr);
    virtual ~TGotoStatement () = default;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
    TSymbol *getLabel () const;
    TExpressionBase *getCondition () const;
    
private:
     void parse (TBlock &);

     TSymbol *label;
     TExpressionBase *condition;
};

inline TSymbol *TGotoStatement::getLabel () const {
    return label;
}

inline TExpressionBase *TGotoStatement::getCondition () const {
    return condition;
}


class TWithStatement: public TStatement {
using inherited = TStatement;
public:
    TWithStatement (TBlock &);
    
    TStatement *getStatement () const;
    
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
    
private:
    void parse (TBlock &);
    
    TStatement *statement;
};

inline TStatement *TWithStatement::getStatement () const {
    return statement;
}

}
