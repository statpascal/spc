#include "statements.hpp"
#include "expression.hpp"
#include "codegenerator.hpp"
#include "compilerimpl.hpp"

#include <algorithm>
#include <limits>

namespace statpascal {

TStatement *TStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    
    declarations.getSymbols ().beginNewTempBlock ();
    
    if (lexer.checkToken (TToken::Begin))
        return compiler.createMemoryPoolObject<TStatementSequence> (declarations);
    if (lexer.checkToken (TToken::If))
        return compiler.createMemoryPoolObject<TIfStatement> (declarations);
    if (lexer.checkToken (TToken::Repeat))
        return TRepeatStatement::parse (declarations);
    if (lexer.checkToken (TToken::While))
        return TWhileStatement::parse (declarations);
    if (lexer.checkToken (TToken::For))
        return TForStatement::parse (declarations);
    if (lexer.checkToken (TToken::Case))
        return compiler.createMemoryPoolObject<TCaseStatement> (declarations);
    if (lexer.checkToken (TToken::Goto))
        return compiler.createMemoryPoolObject<TGotoStatement> (declarations);
    if (lexer.checkToken (TToken::With))
        return compiler.createMemoryPoolObject<TWithStatement> (declarations);
    if (lexer.getToken () == TToken::Identifier || lexer.getToken () == TToken::IntegerConst) {
        std::string s = (lexer.getToken () == TToken::Identifier) ? lexer.getIdentifier () : std::to_string (lexer.getInteger ());
        if (TSymbol *symbol = declarations.getSymbols ().searchSymbol (s))
            if (symbol->checkSymbolFlag (TSymbol::Label)) {
                lexer.getNextToken ();
                return compiler.createMemoryPoolObject<TLabeledStatement> (symbol, declarations);
            }
//        return compiler.createMemoryPoolObject<TSimpleStatement> (declarations);
        return TSimpleStatement::parse (declarations);
    }
    return compiler.createMemoryPoolObject<TEmptyStatement> ();
}

TSymbol *TStatement::createLocalLabel (TBlock &declarations) {
//    TCompilerImpl &compiler = declarations.getCompiler ();
//    return declarations.getSymbols ().addLabel (reinterpret_cast<TBaseGenerator &> (compiler.getCodeGenerator ()).getNextLocalLabel ()).symbol;
    return declarations.getSymbols ().makeLocalLabel ('l');
}

TSymbol *TStatement::createCaseLabel (TBlock &declarations) {
//    TCompilerImpl &compiler = declarations.getCompiler ();
//    return declarations.getSymbols ().addLabel (reinterpret_cast<TBaseGenerator &> (compiler.getCodeGenerator ()).getNextCaseLabel ()).symbol;
    return declarations.getSymbols ().makeLocalLabel ('c');
}

std::vector<TStatement *> TStatement::parseStatementSequence (TBlock &declarations, TToken endToken) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    
    std::vector<TStatement *> statements;
    bool goon;
    do {
        statements.push_back (TStatement::parse (declarations));
        TToken token = lexer.getToken ();
        if (token != endToken && token != TToken::Semicolon) {
            compiler.errorMessage (TCompilerImpl::SyntaxError, "A semicolon or 'end' is missing");
            compiler.recoverPanicMode ({TToken::End, TToken::Semicolon, TToken::Until, TToken::Else, TToken::Begin,
                                        TToken::Procedure, TToken::Function, TToken::Terminator});
        }
        goon = lexer.checkToken (TToken::Semicolon);
    } while (goon);
    return statements;
}


void TEmptyStatement::acceptCodeGenerator (TCodeGenerator &) {
}


TLabeledStatement::TLabeledStatement (TSymbol *label, TBlock &declarations):
  label (label) {
    if (label->getLevel () != declarations.getSymbols ().getLevel ())
        declarations.getCompiler ().errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Label '" + label->getName () + "' is not local");
    if (label->getOffset () == TSymbol::LabelDefined)
        declarations.getCompiler ().errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Label '" + label->getName () + "' is already defined");
    label->setOffset (TSymbol::LabelDefined);
    parse (declarations);
}

TLabeledStatement::TLabeledStatement (TSymbol *label, TStatement *statement):
  label (label), statement (statement) {
    label->setOffset (TSymbol::LabelDefined);
}

void TLabeledStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    compiler.checkToken (TToken::Colon, "':' expected after label");
    statement = TStatement::parse (declarations);
}

void TLabeledStatement::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


void TAssignment::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


void TRoutineCall::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TStatement *TSimpleStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    
    const std::string identifier = lexer.getIdentifier ();
    TExpressionBase *left = TExpression::parse (declarations),
                    *right = nullptr;
                    
    if (lexer.checkToken (TToken::Define))
        right = TExpression::parse (declarations);
        
    if (left) 
        if (right) {	// Assignment
            if (left->isLValueDereference ()) {
                left = static_cast<TLValueDereference *> (left)->getLValue ();
                // check if assignment fo function return temp
                if (left->isFunctionCall ())
                    compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Cannot assign to function result");
                else if (!left->isVectorIndex ()) {	// handled as function call
                    TExpressionBase::performTypeConversion (left->getType (), right, declarations);
                    if (right->isFunctionCall () && (left->getType () == right->getType () || left->getType ()->isVector () || (left->getType ()->isSet () && right->getType ()->isSet ()))
                        && static_cast<TFunctionCall *> (right)->getReturnStorage () && !compiler.getCodeGenerator ().isFunctionCallInlined (*static_cast<TFunctionCall *> (right))) {
                        static_cast<TFunctionCall *> (right)->setReturnStorage (left, declarations);
                        return compiler.createMemoryPoolObject<TRoutineCall> (right);
                    }
                }
            } else if (left->isRoutine ()) {
                const TSymbol *symbol = static_cast<TRoutineValue *> (left)->getSymbol ();
                TBlock *prevBlock = &declarations;
                while (prevBlock)
                    if (prevBlock->getSymbol () && prevBlock->getSymbol ()->getName () == symbol->getName () && (left = prevBlock->returnLValueDeref)) {
                        left = static_cast<TLValueDereference *> (left)->getLValue ();
                        TType *returnType = static_cast<TRoutineType *> (symbol->getType ())->getReturnType ();
                        TExpressionBase::performTypeConversion (returnType, right, declarations);
                        break;
                    } else
                        prevBlock = prevBlock->getParentBlock ();
                if (!prevBlock)
                    compiler.errorMessage (TCompilerImpl::InternalError, "Cannot assign to function name '" + identifier + "' at this position");
            } else
                compiler.errorMessage (TCompilerImpl::InternalError, "Cannot handle simple statement");
            return compiler.createMemoryPoolObject<TAssignment> (left, right);
        } else {	// Procedure/Function call
            if (left->isFunctionCall ())
                static_cast<TFunctionCall *> (left)->ignoreReturnValue (true);
            else if (!TExpressionBase::checkTypeConversion (&stdType.Void, left, declarations))
                compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Assigment or subroutine call expected");
            return compiler.createMemoryPoolObject<TRoutineCall> (left);
        }
    else
        return compiler.createMemoryPoolObject<TEmptyStatement> ();
}



TIfStatement::TIfStatement (TBlock &declarations):
  condition (nullptr), statement1 (nullptr), statement2 (nullptr) {
    parse (declarations);
}

TIfStatement::TIfStatement (TExpressionBase *condition, TStatement *statement):
  condition (condition), statement1 (statement), statement2 (nullptr) {
}

void TIfStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    
    condition = TExpression::parse (declarations);
    if (condition && !TExpressionBase::checkTypeConversion (&stdType.Boolean, condition, declarations))
        compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Boolean required in condition of 'if'-statement");
    compiler.checkToken (TToken::Then, "'then' expected");
    statement1 = TStatement::parse (declarations);
    if (lexer.checkToken (TToken::Else))
        statement2 = TStatement::parse (declarations);
}

void TIfStatement::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TStatementSequence::TStatementSequence (TBlock &declarations) {
    parse (declarations);
}

TStatementSequence::TStatementSequence (std::vector<TStatement *> &&v) {
    statements = std::move (v);
}

void TStatementSequence::appendFront (TStatement *statement) {
    statements.insert (statements.begin (), statement);
}

void TStatementSequence::appendBack (TStatement *statement) {
    statements.push_back (statement);
}

void TStatementSequence::parse (TBlock &declarations) {
    statements = parseStatementSequence (declarations, TToken::End);
    declarations.getCompiler ().getLexer ().checkToken (TToken::End);
}

void TStatementSequence::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TStatement *TRepeatStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TSymbol *label = createLocalLabel (declarations);
    std::vector<TStatement *> statements = parseStatementSequence (declarations, TToken::Until);
    TExpressionBase *condition;
    if (declarations.getCompiler ().getLexer ().checkToken (TToken::Until) && (condition = TExpression::parse (declarations))) {
        TExpressionBase::performTypeConversion (&stdType.Boolean, condition, declarations);
        statements.push_back (compiler.createMemoryPoolObject<TGotoStatement> (label, 
            compiler.createMemoryPoolObject<TPrefixedExpression> (condition, TToken::Not, declarations)));
        return compiler.createMemoryPoolObject<TLabeledStatement> (
            label, compiler.createMemoryPoolObject<TStatementSequence> (std::move (statements)));
    }
    return compiler.createMemoryPoolObject<TEmptyStatement> ();
}


TStatement *TWhileStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TSymbol *l1 = createLocalLabel (declarations),
            *l2 = createLocalLabel (declarations);
    std::vector<TStatement *> statements;
    TExpressionBase *condition = TExpression::parse (declarations);
    if (condition) {
        TExpressionBase::performTypeConversion (&stdType.Boolean, condition, declarations);
        statements.push_back (compiler.createMemoryPoolObject<TGotoStatement> (l2,
            compiler.createMemoryPoolObject<TPrefixedExpression> (condition, TToken::Not, declarations)));

    }
    compiler.checkToken (TToken::Do, "'do' expected in 'while'-statement");
    statements.push_back (TStatement::parse (declarations));
    statements.push_back (compiler.createMemoryPoolObject<TGotoStatement> (l1));
    statements.push_back (compiler.createMemoryPoolObject<TLabeledStatement> (
            l2, compiler.createMemoryPoolObject<TEmptyStatement> ()));
    return compiler.createMemoryPoolObject<TLabeledStatement> (
        l1, compiler.createMemoryPoolObject<TStatementSequence> (std::move (statements)));
}


TCaseStatement::TCaseStatement (TBlock &declarations):
  expression (nullptr), defaultStatement (nullptr) {
    parse (declarations);
}

void TCaseStatement::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

void TCaseStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    
    expression = TExpression::parse (declarations);
    const TType *type = expression ? TExpressionBase::convertBaseType (expression, declarations) : nullptr;
    if (type && !type->isEnumerated ())
        compiler.errorMessage (TCompilerImpl::InvalidType, "Ordinal type expected in 'case'-statement: '" + type->getName () + "' is invalid");
    compiler.checkToken (TToken::Of, "'of' expected in 'case'-statement");
        
    bool parseCase = true;
    minLabel = std::numeric_limits<std::int64_t>::max ();
    maxLabel = std::numeric_limits<std::int64_t>::min ();
    while (parseCase) {
        TLabelList labels;
        TSymbol *jumpLabel = createCaseLabel (declarations);
        do {
            const TSimpleConstant *a = declarations.parseConstExpression (), *b = a;
            if (lexer.checkToken (TToken::Points))
                b = declarations.parseConstExpression ();
            if (a->getType () != type || b->getType () != type)
                compiler.errorMessage (TCompilerImpl::InvalidType, "Type of case label does not match");
            else {
                const std::int64_t aval = a->getInteger (), bval = b->getInteger ();
                for (const TSortedEntry &e: sortedLabels) 
                    if ((aval >= e.label.a && aval <= e.label.b) ||
                        (bval >= e.label.a && bval <= e.label.b)) {
                        compiler.errorMessage (TCompilerImpl::DuplicateCase, "Duplicate case label");
                        break;
                    }
                labels.push_back ({aval, bval});
                TSortedEntry entry {labels.back (), jumpLabel};
                sortedLabels.insert (std::upper_bound (sortedLabels.begin (), sortedLabels.end (), entry, [] (const TSortedEntry &a, const TSortedEntry &b) { return a.label.a < b.label.a; }), entry);
                minLabel = std::min (minLabel, aval);
                maxLabel = std::max (maxLabel, bval);
            }
        } while (lexer.checkToken (TToken::Comma));
        compiler.checkToken (TToken::Colon, "':' expected in 'case'-statement");
        caseList.push_back (TCase {std::move (labels), TStatement::parse (declarations), jumpLabel});
        parseCase = lexer.checkToken (TToken::Semicolon) && !(lexer.getToken () == TToken::Else || lexer.getToken () == TToken::End);
    }
    
    if (lexer.checkToken (TToken::Else))
        defaultStatement = compiler.createMemoryPoolObject<TStatementSequence> (declarations);
    else 
        compiler.checkToken (TToken::End, "'end' expected in 'case'-statement");
}    


TStatement *TForStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    TSymbol *controlVariable;
    bool isIncrement, isBeginConst, isEndConst;
    std::int64_t beginValue = 0, endValue = 0;
    
    if (!(controlVariable = declarations.checkVariable ()))
        lexer.getNextToken ();
    // TODO: Check if control variable is local
    compiler.checkToken (TToken::Define, "':=' required in 'for'-statement");
    TExpressionBase *begin = TExpression::parse (declarations);
    if (lexer.checkToken (TToken::To))
        isIncrement = true;
    else if (lexer.checkToken (TToken::Downto))
        isIncrement = false;
    else
        compiler.errorMessage (TCompilerImpl::SyntaxError, "'to' or 'downto' expected in 'for'-loop");
    TExpressionBase *end = TExpression::parse (declarations);
    if (controlVariable) {
        TType *controlType = controlVariable->getType ();
        if (controlVariable->checkSymbolFlag (TSymbol::Constant) || !controlType->isEnumerated ())
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Control variable of 'for'-loop must be of an ordinal type");
        if (begin) {
            if ((isBeginConst = begin->isConstant ()))
                beginValue = static_cast<TConstantValue *> (begin)->getConstant ()->getInteger ();
            TExpressionBase::performTypeConversion (controlType, begin, declarations);
        }
        if (end) {
            if ((isEndConst = end->isConstant ())) 
                endValue = static_cast<TConstantValue *> (end)->getConstant ()->getInteger ();
            TExpressionBase::performTypeConversion (controlType, end, declarations);
        }
    }
    compiler.checkToken (TToken::Do, "'do' expected in 'for'-loop");
    TStatement *bodyStatement = TStatement::parse (declarations);
    
    if (compiler.getErrorFlag () || (isEndConst && isBeginConst && ((endValue < beginValue && isIncrement) || (endValue > beginValue && !isIncrement))))
        return compiler.createMemoryPoolObject<TEmptyStatement> ();
  
//   Collides with range check in PCODE - disable for initial assignment  
        
//    if (isBeginConst && isEndConst)
//        begin = TExpressionBase::createConstant (isIncrement ? beginValue - 1 : beginValue + 1, begin->getType (), declarations);

    // transform for loop
    std::vector<TStatement *> statements;
    TExpressionBase *controlVariableLValue = compiler.createMemoryPoolObject<TVariable> (controlVariable, declarations);
    TExpressionBase *controlVariableAccess = compiler.createMemoryPoolObject<TLValueDereference> (controlVariableLValue);
    statements.push_back (compiler.createMemoryPoolObject<TAssignment> (controlVariableLValue, begin));
    
    if (!end->isConstant ()) {
//    if (!isEndConst) {
        TSymbol *endTemp = declarations.getSymbols ().addVariable ("$" + controlVariable->getName (), controlVariable->getType ()).symbol;
        TExpressionBase *endVar = compiler.createMemoryPoolObject<TVariable> (endTemp, declarations);
        statements.push_back (compiler.createMemoryPoolObject<TAssignment> (endVar, end));
        end = compiler.createMemoryPoolObject<TLValueDereference> (endVar);
    }
    
    TSymbol *beginLabel = createLocalLabel (declarations),
            *bodyLabel = createLocalLabel (declarations),
            *endLabel = createLocalLabel (declarations);
    if (!(isBeginConst && isEndConst))
        statements.push_back (compiler.createMemoryPoolObject<TGotoStatement> (endLabel, 
            compiler.createMemoryPoolObject<TExpression> (controlVariableAccess, end, isIncrement ? TToken::GreaterThan : TToken::LessThan, &stdType.Boolean)));
//    if (!(isBeginConst && isEndConst))
        statements.push_back (compiler.createMemoryPoolObject<TGotoStatement> (bodyLabel));
    std::vector<TExpressionBase *> args {controlVariableLValue};
    statements.push_back (compiler.createMemoryPoolObject<TLabeledStatement> (
        beginLabel, compiler.createMemoryPoolObject<TRoutineCall> (
            compiler.createMemoryPoolObject<TPredefinedRoutine> (isIncrement ? TPredefinedRoutine::Inc : TPredefinedRoutine::Dec, controlVariable->getType (), std::move (args)))));
    statements.push_back (compiler.createMemoryPoolObject<TLabeledStatement> (bodyLabel, bodyStatement));
    statements.push_back (compiler.createMemoryPoolObject<TGotoStatement> (beginLabel, 
        compiler.createMemoryPoolObject<TExpression> (controlVariableAccess, end, TToken::NotEqual, &stdType.Boolean)));
    statements.push_back (compiler.createMemoryPoolObject<TLabeledStatement> (endLabel,
        compiler.createMemoryPoolObject<TEmptyStatement> ()));
        
    return compiler.createMemoryPoolObject<TStatementSequence> (std::move (statements));
}


TGotoStatement::TGotoStatement (TBlock &declarations): 
  TGotoStatement (nullptr) {
    parse (declarations);
}

TGotoStatement::TGotoStatement (TSymbol *label, TExpressionBase *condition):
  label (label), condition (condition) {
}

void TGotoStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
    
    if (lexer.getToken () == TToken::Identifier || lexer.getToken () == TToken::IntegerConst) {
        std::string s = (lexer.getToken () == TToken::Identifier) ? lexer.getIdentifier () : std::to_string (lexer.getInteger ());
        compiler.getLexer ().getNextToken ();
        // !!!! searchSymbol mit Label
        if (!(label = declarations.getSymbols ().searchSymbol (s)) || !label->checkSymbolFlag (TSymbol::Label))
            compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Invalid use of '" + s + "' in goto");
        else if (label->getLevel () != declarations.getSymbols ().getLevel ())
            compiler.errorMessage (TCompilerImpl::InvalidUseOfSymbol, "Label '" + s + "' is not local");
        else if (label->getOffset () != TSymbol::LabelDefined)
            label->setOffset (TSymbol::UndefinedLabelUsed);
    } else
        compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Label expected in 'goto'");
}

void TGotoStatement::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TWithStatement::TWithStatement (TBlock &declarations):
  statement (nullptr) {
    parse (declarations);
}

void TWithStatement::parse (TBlock &declarations) {
    TCompilerImpl &compiler = declarations.getCompiler ();
    TLexer &lexer = compiler.getLexer ();
        
    std::size_t count = 0;
    do 
        if (TExpressionBase *expr = TExpression::parse (declarations)) {
            if (expr->getType ()->isRecord ()) {
                ++count;
                declarations.pushActiveRecord (expr);
            } else
                compiler.errorMessage (TCompilerImpl::InvalidType, "Record type required in 'with'-statement");
        }
    while (lexer.checkToken (TToken::Comma));
    compiler.checkToken (TToken::Do, "'do' expected in 'with'-statement");
    statement = TStatement::parse (declarations);
    while (count--)
        declarations.popActiveRecord ();
}

void TWithStatement::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    statement->acceptCodeGenerator (codeGenerator);
}

}
