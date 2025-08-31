#include "expression.hpp"
#include "codegenerator.hpp"
#include "predefined.hpp"

#include <iostream>
#include <map>

namespace statpascal {

TExpressionBase::TExpressionBase ():
  type (nullptr) {
}

bool TExpressionBase::isLValue () const {
    return false;
}

bool TExpressionBase::isLValueDereference () const {
    return false;
}

bool TExpressionBase::isConstant () const {
    return false;
}

bool TExpressionBase::isSymbol () const {
    return false;
}

bool TExpressionBase::isReference () const {
    return false;
}

bool TExpressionBase::isRoutine () const {
    return false;
}

bool TExpressionBase::isFunctionCall () const {
    return false;
}

bool TExpressionBase::isTypeCast ()  const {
    return false;
}

bool TExpressionBase::isVectorIndex () const {
    return false;
}

bool TExpressionBase::createFunctionCall (TExpressionBase *&base, TBlock &block, bool recursive) {
    bool callCreated = false;
    if (base) {
        if (base->isRoutine ())
            static_cast<TRoutineValue *> (base)->resolveCall (std::vector<TExpressionBase *> (), block);
        
        TType *t = base->getType ();
        while (t && t->isRoutine () && !(static_cast<TRoutineType *> (t)->getParameter ().size ())) {
            callCreated = true;
            base = block.getCompiler ().createMemoryPoolObject<TFunctionCall> (base, std::vector<TExpressionBase *> (), block, true);
            t = base->getType ();
            if (!recursive) 
                return true;
        }
    }
    return callCreated;
}

TFunctionCall *TExpressionBase::createRuntimeCall (const std::string &name, TType *resultType, std::vector<TExpressionBase *> &&args, TBlock &block, bool checkParameter) {
    TCompilerImpl &compiler = block.getCompiler ();
    TFunctionCall *expr = compiler.createMemoryPoolObject<TFunctionCall> (
        compiler.createMemoryPoolObject<TRoutineValue> (name, block), std::move (args), block, checkParameter);
    if (resultType)
        expr->setType (resultType);
    return expr;
}

TExpressionBase *TExpressionBase::createInt64Constant (std::int64_t val, TBlock &block) {
    return createConstant (val, &stdType.Int64, block);
}

template<typename T> TExpressionBase *TExpressionBase::createConstant (T val, TType *type, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    return compiler.createMemoryPoolObject<TConstantValue> (compiler.createMemoryPoolObject<TSimpleConstant> (val, type));
}

template TExpressionBase *TExpressionBase::createConstant<std::int64_t> (std::int64_t, TType *, TBlock &);

bool TExpressionBase::mergeConstants (TExpressionBase *&left, TExpressionBase *right, TType *type, TToken operation, TBlock &block) {
    if (left->isConstant () && right->isConstant ()) {
        if (type == &stdType.Int64) {
            std::int64_t a = static_cast<TConstantValue *> (left)->getConstant ()->getInteger (),
                         b = static_cast<TConstantValue *> (right)->getConstant ()->getInteger ();
            switch (operation) {
                case TToken::Add:
                    a += b; break;
                case TToken::Sub:
                    a -= b; break;
                case TToken::Or:
                    a |= b; break;
                case TToken::Xor:
                    a ^= b; break;
                case TToken::Mul:
                    a *= b; break;
                case TToken::DivInt:
                    a /= b; break;
                case TToken::Shl:
                    a <<= b; break;
                case TToken::Shr:
                    a >>= b; break; 
                case TToken::Mod:
                     a %= b; break;
                case TToken::And:
                     a &= b; break;
                default:
                    return false;
            }
            left = createInt64Constant (a, block);
            return true;
        }
    }
    return false;
}

bool TExpressionBase::evaluateConstant (TExpressionBase *&expr, TType *type, TToken operation, TBlock &block) {
    TExpressionBase *const exprIn = expr;
    
    if (expr->isConstant ()) {
        const TSimpleConstant *constValue = static_cast<TConstantValue *> (expr)->getConstant ();
        if (operation == TToken::Sub) {
            if (type == &stdType.Int64)
                expr = createInt64Constant (-constValue->getInteger (), block);
            else if (type == &stdType.Real)
                expr = createConstant (-constValue->getDouble (), type, block);
        }
        if (operation == TToken::Not) {
            if (type == &stdType.Int64)
                expr = createInt64Constant (~constValue->getInteger (), block);
            else if (type == &stdType.Boolean)
                expr = createConstant (static_cast<std::int64_t> (!constValue->getInteger ()), type, block);
        }
    }
    return expr != exprIn;
}

TExpressionBase *TExpressionBase::createVariableAccess (const std::string &name, TBlock &block) {
    return block.getCompiler ().createMemoryPoolObject<TLValueDereference> (
               block.getCompiler ().createMemoryPoolObject<TVariable> (block.getSymbols ().searchSymbol (name), block));
}

TType *TExpressionBase::convertBaseType (TExpressionBase *&expression, TBlock &block) {
    createFunctionCall (expression, block, true);
    TType *t = expression->getType ();
    if (t && t->isSubrange ()) {
        TType *destType = t->getBaseType ();
        expression = block.getCompiler ().createMemoryPoolObject<TTypeCast> (destType, expression);
        return destType;
    }
    if (t && t == &stdType.Single) {
        TType *destType = &stdType.Real;
        expression = block.getCompiler ().createMemoryPoolObject<TTypeCast> (destType, expression);
        return destType;
    }
    return t;
}

bool TExpressionBase::checkTypeConversion (TType *required, TExpressionBase *&expr, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    if (!required || !expr->getType ())
        return true;
    if (required == expr->getType ())
        return true;
        
    if (required->isShortString () && expr->getType ()->isShortString ())
        return true;
        
    if (required->isPointer () && expr->getType ()->isShortString ())
        return true;
        
    if (expr->getType () == &stdType.UnresOverload && expr->isRoutine () && required->isRoutine ())
        return static_cast<TRoutineValue *> (expr)->resolveConversion (static_cast<TRoutineType *> (required));


    if (required->isRoutine () && expr->getType ()->isRoutine () &&
        static_cast<TRoutineType *> (required)->matchesOverload (static_cast<TRoutineType *> (expr->getType ())))
            return true;
            
    bool requiredSubrange = required->isSubrange (),
         exprSubrange = expr->getType ()->isSubrange ();
         
    if (requiredSubrange) {
        if ((exprSubrange && required->getBaseType () != expr->getType ()->getBaseType ()) ||
            (!exprSubrange && !checkTypeConversion (required->getBaseType (), expr, block)))
            return false;
        // TODO: range check for constants performed here !!!!
        expr = compiler.createMemoryPoolObject<TTypeCast> (required, expr);
        return true;
    } else if (exprSubrange) {
        expr = compiler.createMemoryPoolObject<TTypeCast> (expr->getType ()->getBaseType (), expr);
        return checkTypeConversion (required, expr, block);
    }
    
    if (required == &stdType.Single && checkTypeConversion (&stdType.Real, expr, block)) {
        expr = compiler.createMemoryPoolObject<TTypeCast> (required, expr);
        return true;
    }

    if (createFunctionCall (expr, block, false)) 
        return checkTypeConversion (required, expr, block);
    
    if (required == &stdType.Real && (expr->getType () == &stdType.Int64 || expr->getType () == &stdType.Single)) {
        if (expr->isConstant () && expr->getType () == &stdType.Int64)
            expr = createConstant<double> (static_cast<TConstantValue *> (expr)->getConstant ()->getInteger (), &stdType.Real, block);
        else
            expr = compiler.createMemoryPoolObject<TTypeCast> (required, expr);
        return true;
    }

    if (required == &stdType.String && expr->getType () == &stdType.Char) {
        expr = createRuntimeCall ("__str_char", required, {expr}, block, false);
        return true;
    }
    
    if (required->isShortString () && expr->getType () == &stdType.Char) {
        expr = createRuntimeCall ("__short_str_char", required, {expr}, block, false);
        return true;
    }
    
    if (required->isPointer () && expr->getType ()->isPointer ()) 
        return required == &stdType.GenericPointer || expr->getType () == &stdType.GenericPointer ||
               required->getBaseType () == expr->getType ()->getBaseType ();
               
    if (required->isSet () && expr->getType ()->isSet ()) {
        TType *leftBase = static_cast<TSetType *> (required)->getBaseType (),
              *rightBase = static_cast<TSetType *> (expr->getType ())->getBaseType ();
        if (!rightBase)
            return true;
        if (leftBase->isSubrange ())
            leftBase = leftBase->getBaseType ();
        if (rightBase->isSubrange ())
            rightBase = rightBase->getBaseType ();
        return rightBase == leftBase;
    }
               
    if (required->isVector ())
        return checkTypeConversionVector (required, expr, block);
        
    // cast function to void; ignoring return type
    if (required == &stdType.Void && (expr->isFunctionCall () || (expr->isTypeCast () && static_cast<TTypeCast *> (expr)->getExpression ()->isFunctionCall ()))) {
        static_cast<TFunctionCall *> (expr)->ignoreReturnValue (true);
        return true;
    }
    
    if (required->isPointer () && required->getBaseType () == &stdType.Char &&
        checkTypeConversion (&stdType.String, expr, block)) {
        expr = compiler.createMemoryPoolObject<TTypeCast> (required, expr);
        return true;
    }
               
    return false;
}

bool TExpressionBase::performTypeConversion (TType *required, TExpressionBase *&expr, TBlock &block) {
    bool f = checkTypeConversion (required, expr, block);
    if (!f)
        block.getCompiler ().errorMessage (TCompilerImpl::IncompatibleTypes, "Cannot convert \'" + expr->getType ()->getName () + "\' to \'" + required->getName () + "\'");
    return f;
}

bool TExpressionBase::checkTypeConversionVector (TType *required, TExpressionBase *&expression, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    TVectorType *requiredVectorType = dynamic_cast<TVectorType *> (required);
    
//    if (expression->getType () == &stdType.GenericVector)
//        return true;

// TODO: lift non-scalars: better assignment to vec [0]
    
    if (checkTypeConversion (requiredVectorType->getBaseType (), expression, block)) {
        // Flag: Scalar->Vector Lift ware sinnvoll
        TBaseGenerator &codeGenerator = static_cast<TBaseGenerator &> (compiler.getCodeGenerator ());
        if (expression->getType ()->isVector () || expression->getType ()->isString ()) 
            expression = createRuntimeCall (expression->getType ()->isVector () ? "__make_vec_vec" : "__make_vec_str", required, {
                createInt64Constant (codeGenerator.lookupAnyManager (expression->getType ()).runtimeIndex, block), 
                createVariableAccess (TConfig::globalRuntimeDataPtr, block), 
                expression}, 
                block, false);
        else
            expression = createRuntimeCall (expression->getType ()->isSingle () || expression->getType ()->isReal () ? "__make_vec_dbl" : "__make_vec_int", required, 
                {createInt64Constant (expression->getType ()->getSize (), block), expression}, block, false);
        return true;
    }
    
    if (expression->getType ()->isVector ()) {
        const TType *destBaseType = requiredVectorType->getBaseType (),
                    *srcBaseType = expression->getType ()->getBaseType ();
        const TStdType::TScalarTypeCode
            tcs = TStdType::getScalarTypeCode (srcBaseType),
            tcd = TStdType::getScalarTypeCode (destBaseType);
                    
        if (destBaseType == srcBaseType) {
            // Flag: Nothing to do ware sinnvoll:
//            expression = compiler.createMemoryPoolObject<TTypeCast> (requiredVectorType, expression);
            return true;
        }
            
        if (destBaseType->isSubrange ())
            destBaseType = destBaseType->getBaseType ();
        if (srcBaseType->isSubrange ())
            srcBaseType = srcBaseType->getBaseType ();
            
        if (destBaseType == srcBaseType || 
           ((destBaseType == &stdType.Real || destBaseType == &stdType.Single) && 
            (srcBaseType == &stdType.Int64 || srcBaseType == &stdType.Real || srcBaseType == &stdType.Single))) {
            expression = createRuntimeCall ("__vec_conv", requiredVectorType, {expression, createInt64Constant (tcs, block), createInt64Constant (tcd, block)}, block, false);
            return true;
        }
    }
    return false;
}

TType *TExpressionBase::getVectorBaseType (TExpressionBase *expression, TBlock &block) {
    TType *type = expression->getType ();
    if (!type->isVector ())
        return nullptr;
    type = type->getBaseType ();
    if (type->isSubrange ())
        type = type->getBaseType ();
    else if (type == &stdType.Single)
        type = &stdType.Real;
    return type;
}

TType *TExpressionBase::retrieveVectorAndBaseType (TExpressionBase *&expression, TBlock &block) {
    if (!expression->getType ()->isVector ())
        checkTypeConversion (block.getCompiler ().createMemoryPoolObject<TVectorType> (expression->getType ()), expression, block);
//        expression = block.getCompiler ().createMemoryPoolObject<TTypeCast> (block.getCompiler ().createMemoryPoolObject<TVectorType> (expression->getType ()), expression);
    return getVectorBaseType (expression, block);
}

TType *TExpressionBase::checkVectorOperatorTypes (TExpressionBase *&left, TExpressionBase *&right, TToken operation, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    TType *leftBaseType = retrieveVectorAndBaseType (left, block),
          *rightBaseType = retrieveVectorAndBaseType (right, block);
          
    switch (operation) {
        case TToken::And:
        case TToken::Or:
        case TToken::Xor:
            if (leftBaseType == rightBaseType) {
                if (leftBaseType == &stdType.Int64)
                    return &stdType.Int64Vector;
                if (leftBaseType == &stdType.Boolean)
                    return &stdType.BooleanVector;
            }
            break;
            
        case TToken::DivInt:
        case TToken::Mod:
        case TToken::Shl:
        case TToken::Shr:
            if (leftBaseType == rightBaseType && leftBaseType == &stdType.Int64)
                return &stdType.Int64Vector;
            break;
            
        case TToken::Add:
        case TToken::Sub:
        case TToken::Mul:
            if (leftBaseType == &stdType.Int64 && rightBaseType == &stdType.Int64)
                return &stdType.Int64Vector;
            // fall through
        case TToken::Div:
            if ((leftBaseType == &stdType.Int64 || leftBaseType == &stdType.Real) && (rightBaseType == &stdType.Int64 || rightBaseType == &stdType.Real))
                return &stdType.RealVector;
            break;
            
        case TToken::Equal:
        case TToken::NotEqual:
        case TToken::GreaterThan: 
        case TToken::LessThan: 
        case TToken::GreaterEqual:
        case TToken::LessEqual:
            if (leftBaseType->isEnumerated () && leftBaseType == rightBaseType)
                return &stdType.BooleanVector;
            if ((leftBaseType == &stdType.Int64 || leftBaseType == &stdType.Real) && (rightBaseType == &stdType.Int64 || rightBaseType == &stdType.Real))
                return &stdType.BooleanVector;
            break;
        default:
            break;
    }
    
    compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Invalid types in expression");
    return nullptr;
}

TType *TExpressionBase::checkSetOperatorTypes (TExpressionBase *&left, TExpressionBase *&right, TToken operation, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    
    TSetType *rightSetType = dynamic_cast<TSetType *> (right->getType ());
    TEnumeratedType *rightMemberType = rightSetType->getBaseType (),
                    *rightBaseType = (rightMemberType && rightMemberType->isSubrange ()) ?
                                     static_cast<TEnumeratedType *> (rightMemberType->getBaseType ()) :
                                     rightMemberType;

    if (operation == TToken::In) {
        if (rightBaseType == left->getType () || (!rightBaseType && left->getType ()->isEnumerated ()))
            return &stdType.Boolean;
        else {
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Invalid types used with 'in' operator");
            return nullptr;
        }
    }
    
    if (left->getType ()->isSet ()) {
        TSetType *leftSetType = static_cast<TSetType *> (left->getType ());
        TEnumeratedType *leftMemberType = leftSetType->getBaseType (),
                        *leftBaseType = (leftMemberType && leftMemberType->isSubrange ()) ?
                                        static_cast<TEnumeratedType *> (leftMemberType->getBaseType ()) :
                                        leftMemberType;
        if (leftBaseType == rightBaseType || !rightBaseType || !leftBaseType)
            switch (operation) {
                case TToken::Add:
                case TToken::Sub:
                case TToken::Mul:
                    if (!rightBaseType)	// empty set constant
                        return leftSetType;
                    else if (!leftBaseType)
                        return rightSetType;
                    else 
                        return compiler.createMemoryPoolObject<TSetType> (
                            compiler.createMemoryPoolObject<TSubrangeType>
                                (std::string (), leftBaseType, 
                                 std::min (leftMemberType->getMinVal (), rightMemberType->getMinVal ()),
                                 std::max (leftMemberType->getMaxVal (), rightMemberType->getMaxVal ())));
                case TToken::Equal:
                case TToken::GreaterThan:
                case TToken::LessThan:
                case TToken::GreaterEqual:
                case TToken::LessEqual:
                case TToken::NotEqual:
                    return &stdType.Boolean;
                default:
                    compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Invalid set operation");            
            }
    } else  
        compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Invalid types used in set operator");
    return nullptr;
}

TType *TExpressionBase::checkOperatorTypes (TExpressionBase *&left, TExpressionBase *&right, TToken operation, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    if (!left || !right)
        return nullptr;
        
    TType *ltype = convertBaseType (left, block),
          *rtype = convertBaseType (right, block);
    if (!ltype || !rtype)
        return nullptr;

    if (ltype->isVector () || rtype->isVector ())
        return checkVectorOperatorTypes (left, right, operation, block);
    if (rtype->isSet ())
        return checkSetOperatorTypes (left, right, operation, block);
        
    if (ltype == &stdType.Int64 && (rtype == &stdType.Real || operation == TToken::Div)) {
        left = block.getCompiler ().createMemoryPoolObject<TTypeCast> (&stdType.Real, left);
        ltype = &stdType.Real;
    }
    if ((ltype == &stdType.Real || operation == TToken::Div) && rtype == &stdType.Int64) {
        right = block.getCompiler ().createMemoryPoolObject<TTypeCast> (&stdType.Real, right);
        rtype = &stdType.Real;
    }
    if ((ltype->isShortString () || ltype == &stdType.String) && rtype == &stdType.Char) {
        checkTypeConversion (ltype, right, block);
        rtype = ltype;
    }
    if (ltype == &stdType.Char && (rtype->isShortString () || rtype == &stdType.String)) {
        checkTypeConversion (rtype, left, block);
        ltype = rtype;
    }

    switch (operation) {
        case TToken::In:
            break;
        case TToken::And:
        case TToken::Or:
        case TToken::Xor:
            if (ltype == rtype && (ltype == &stdType.Int64 || ltype == &stdType.Boolean))
                return ltype;
            break;
        case TToken::DivInt:
        case TToken::Mod:
        case TToken::Shl:
        case TToken::Shr:
            if (ltype == rtype && ltype == &stdType.Int64)
                return ltype;
            break;
        case TToken::Add:
            if (ltype->isShortString () && rtype->isShortString ())
                return &stdType.ShortString;
            if (ltype == rtype && (ltype == &stdType.Int64 || ltype == &stdType.Real || ltype == &stdType.String))
                return ltype;
            if (ltype->isPointer () && rtype == &stdType.Int64)
                return ltype;
            break;
        case TToken::Sub:
            if (ltype->isPointer () && rtype->isPointer () && ltype->getBaseType () == rtype->getBaseType ())
                return &stdType.Int64;
            // fall through
        case TToken::Mul:
            if (ltype == rtype && (ltype == &stdType.Int64 || ltype == &stdType.Real))
                return ltype;
            break;
        case TToken::Div:
            if (ltype == rtype && ltype == &stdType.Real)
                return &stdType.Real;
            break;
            
        case TToken::Equal:
        case TToken::NotEqual:
        case TToken::GreaterThan: 
        case TToken::LessThan: 
        case TToken::GreaterEqual:
        case TToken::LessEqual:
            if (ltype == rtype && (ltype->isEnumerated () || ltype->isReal () || ltype->isString ()))
                return &stdType.Boolean;
            if (ltype->isShortString () && rtype->isShortString ())
                return &stdType.Boolean;
            if (ltype->isPointer () && rtype->isPointer ())
                if (ltype == &stdType.GenericPointer || rtype == &stdType.GenericPointer ||
                    ltype->getBaseType () == rtype->getBaseType ())
                    return &stdType.Boolean;
            break;
        default:
            break;
    }

    compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Invalid types in expression");
    return nullptr;
}

TType *TExpressionBase::getType () const {
    return type;
}

void TExpressionBase::setType (TType *t) {
    type = t;
}


/* TTypeCast can be:
   subrange -> basetye
   basetype -> subrange
   long -> real
   type -> vector of type
*/   

TTypeCast::TTypeCast (TType *required, TExpressionBase *base):
  required (required), base (base) {
    setType (required);
}

void TTypeCast::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

bool TTypeCast::isTypeCast () const {
    return true;
}


TExpression::TExpression (TExpressionBase *left_para, TExpressionBase *right_para, TToken operation, TType *type):
  left (left_para), right (right_para), operation (operation) {
    setType (type);
}

TExpressionBase *TExpression::parse (TBlock &block) {
    static const std::map<TToken, std::string> setRuntimeFunc = {
        {TToken::Equal, 	"__set_equal"},
        {TToken::NotEqual,	"__set_not_equal"},
        {TToken::LessEqual,	"__set_sub"},
        {TToken::GreaterEqual,	"__set_super"},
        {TToken::LessThan,	"__set_sub_not_equal"},
        {TToken::GreaterThan,	"__set_super_not_equal"}
    };
    static const std::map<TToken, std::string> strRuntimeFunc = {
        {TToken::Equal, 	"__str_equal"},
        {TToken::NotEqual,	"__str_not_equal"},
        {TToken::LessEqual,	"__str_less_equal"},
        {TToken::GreaterEqual,	"__str_greater_equal"},
        {TToken::LessThan,	"__str_less"},
        {TToken::GreaterThan,	"__str_greater"}
    };
    static const std::map<TToken, std::string> shortStrRuntimeFunc = {
        {TToken::Equal, 	"__short_str_equal"},
        {TToken::NotEqual,	"__short_str_not_equal"},
        {TToken::LessEqual,	"__short_str_less_equal"},
        {TToken::GreaterEqual,	"__short_str_greater_equal"},
        {TToken::LessThan,	"__short_str_less"},
        {TToken::GreaterThan,	"__short_str_greater"}
    };
    static const std::map<TToken, std::string> vecRuntimeFunc = {
        {TToken::Equal, 	"__vec_equal"},
        {TToken::NotEqual,	"__vec_not_equal"},
        {TToken::LessEqual,	"__vec_less_equal"},
        {TToken::GreaterEqual,	"__vec_greater_equal"},
        {TToken::LessThan,	"__vec_less"},
        {TToken::GreaterThan,	"__vec_greater"}
    };
    
    TCompilerImpl &compiler = block.getCompiler ();
    TExpressionBase *left = TSimpleExpression::parse (block);
    TToken operation = block.getLexer ().getToken ();
    while (operation == TToken::Equal || operation == TToken::GreaterThan || operation == TToken::LessThan ||
           operation == TToken::GreaterEqual || operation == TToken::LessEqual || operation == TToken::NotEqual ||
           operation == TToken::In) {
        block.getLexer ().getNextToken ();
        TExpressionBase *right = TSimpleExpression::parse (block);
        if (left && right)
            if (TType *type = checkOperatorTypes (left, right, operation, block)) {
                if (operation == TToken::In) 
                    left = createRuntimeCall ("__in_set", type, {compiler.createMemoryPoolObject<TTypeCast> (&stdType.Int64, left), right}, block, false);
                else if (left->getType ()->isSet ()) {
                    left = createRuntimeCall (setRuntimeFunc.at (operation), type, {left, right}, block, false);
                } else if (left->getType () == &stdType.String)
                    left = createRuntimeCall (strRuntimeFunc.at (operation), type, {left, right}, block, true);
                else if (left->getType ()->isShortString ())
                    left = createRuntimeCall (shortStrRuntimeFunc.at (operation), type, {left, right}, block, true);
                else if (left->getType ()->isVector ()) {
                    const TStdType::TScalarTypeCode
                        tca = TStdType::getScalarTypeCode (static_cast<TVectorType *> (left->getType ())->getBaseType ()),
                        tcb = TStdType::getScalarTypeCode (static_cast<TVectorType *> (right->getType ())->getBaseType ());
                    left = createRuntimeCall (vecRuntimeFunc.at (operation), type, {left, right, createInt64Constant (tca, block), createInt64Constant (tcb, block)}, block, false);
                } else
                    left = compiler.createMemoryPoolObject<TExpression> (left, right, operation, type);
            }
        operation = block.getLexer ().getToken ();
    }
    return left;
}

void TExpression::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TPrefixedExpression::TPrefixedExpression (TExpressionBase *base_para, TToken operation, TBlock &block):
  base (base_para), operation (operation) {
    TCompilerImpl &compiler = block.getCompiler ();
    convertBaseType (base, block);
    TType *type = base->getType ();
    setType (type);
    
    bool typeOK = false;
    switch (operation) {
        case TToken::Sub:
            typeOK = type == &stdType.Int64 || type == &stdType.Real;
            break;
        case TToken::Not:
            typeOK = type == &stdType.Int64 || type == &stdType.Boolean;
            break;
        default:
            break;
    }
    if (!typeOK)
        compiler.errorMessage (TCompilerImpl::InvalidType, "Prefix applied to invalid type");
}

void TPrefixedExpression::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TSimpleExpression::TSimpleExpression (TExpressionBase *left_para, TExpressionBase *right_para, TToken operation, TType *type):
  left (left_para), right (right_para), operation (operation) {
    setType (type);
}

TExpressionBase *TSimpleExpression::parse (TBlock &block) {
    static const std::map<TToken, std::string> vecRuntimeFunc = {
        {TToken::Add, 	"__vec_add"},
        {TToken::Sub,	"__vec_sub"},
        {TToken::Or,	"__vec_or"},
        {TToken::Xor,	"__vec_xor"}
    };
    
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    lexer.checkToken (TToken::Add);	// skip + if present    
    bool signPresent = lexer.checkToken (TToken::Sub);
    TExpressionBase *left = TTerm::parse (block);
    if (signPresent && left) 
        if (!evaluateConstant (left, left->getType (), TToken::Sub, block))
            left = compiler.createMemoryPoolObject<TPrefixedExpression> (left, TToken::Sub, block);
    
    while (lexer.getToken () == TToken::Add || lexer.getToken () == TToken::Sub || lexer.getToken () == TToken::Or || lexer.getToken () == TToken::Xor) {
        TToken operation = lexer.getToken ();
        lexer.getNextToken ();
        TExpressionBase *right = TTerm::parse (block);
        if (left && right)
            if (TType *type = checkOperatorTypes (left, right, operation, block)) {
                if (type->isSet ()) {
                    left = createRuntimeCall (operation == TToken::Add ? "__set_union" : "__set_diff", type, {left, right}, block, false);
                } else if (type->isVector ()) {
                    const TStdType::TScalarTypeCode
                        tca = TStdType::getScalarTypeCode (static_cast<TVectorType *> (left->getType ())->getBaseType ()),
                        tcb = TStdType::getScalarTypeCode (static_cast<TVectorType *> (right->getType ())->getBaseType ());
                    left = createRuntimeCall (vecRuntimeFunc.at (operation), type, {left, right, createInt64Constant (tca, block), createInt64Constant (tcb, block)}, block, false);
                } else if (type->isString ()) 
                    left = createRuntimeCall ("__str_concat", type, {left, right}, block, false);
                else if (type->isShortString ())
                    left = createRuntimeCall ("__short_str_concat", type, {left, right}, block, false);
                else if (!mergeConstants (left, right, type, operation, block))
                    left = compiler.createMemoryPoolObject<TSimpleExpression> (left, right, operation, type);
            }
    }
    return left;
}

void TSimpleExpression::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TTerm::TTerm (TExpressionBase *left_para, TExpressionBase *right_para, TToken operation, TType *type):
  left (left_para), right (right_para), operation (operation) {
    setType (type);
}

TExpressionBase *TTerm::parse (TBlock &block) {
    static const std::map<TToken, std::string> vecRuntimeFunc = {
        {TToken::Mul, 		"__vec_mul"},
        {TToken::Div,		"__vec_div"},
        {TToken::DivInt,	"__vec_div"},
        {TToken::Mod,		"__vec_mod"},
        {TToken::Shl,		"__vec_shl"},
        {TToken::Shr,		"__vec_shr"},
        {TToken::And,		"__vec_and"}
    };
    
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    TExpressionBase *left = TFactor::parse (block);
    TToken operation = lexer.getToken ();
    while (operation == TToken::Mul || operation == TToken::Div || operation == TToken::DivInt || operation == TToken::Shl || operation == TToken::Shr ||
           operation == TToken::Mod || operation == TToken::And) {
        lexer.getNextToken ();
        TExpressionBase *right = TFactor::parse (block);
        if (left && right)
            if (TType *type = checkOperatorTypes (left, right, operation, block)) {
                if (type->isSet ()) {
                    left = createRuntimeCall ("__set_intersection", type, {left, right}, block, false);
                } else if (type->isVector ()) {
                    const TStdType::TScalarTypeCode
                        tca = TStdType::getScalarTypeCode (static_cast<TVectorType *> (left->getType ())->getBaseType ()),
                        tcb = TStdType::getScalarTypeCode (static_cast<TVectorType *> (right->getType ())->getBaseType ());
                    left = createRuntimeCall (vecRuntimeFunc.at (operation), type, {left, right, createInt64Constant (tca, block), createInt64Constant (tcb, block)}, block, false);
                } else if (!mergeConstants (left, right, type, operation, block))
                    left = compiler.createMemoryPoolObject<TTerm> (left, right, operation, type);
            }
        operation = lexer.getToken ();
    }
    return left;
}

void TTerm::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TExpressionBase *TFactor::parse (TBlock &block) {
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    TExpressionBase *expr = nullptr;
    const TToken token = lexer.getToken ();
    if (token == TToken::Not) {
        lexer.getNextToken ();
        if ((expr = TFactor::parse (block)))
            if (!evaluateConstant (expr, expr->getType (), TToken::Not, block))
                expr = compiler.createMemoryPoolObject<TPrefixedExpression> (expr, TToken::Not, block);
    } else if (token == TToken::Identifier)
        expr = parseIdentifier (block);
    else if (token == TToken::IntegerConst || token == TToken::RealConst || token == TToken::CharConst || token == TToken::StringConst || token == TToken::SizeOf) {
        expr = compiler.createMemoryPoolObject<TConstantValue> (block.parseConstantLiteral ());
#ifndef CREATE_9900        
        if (token == TToken::StringConst)
            expr = createRuntimeCall ("__str_make", nullptr, {expr, TExpressionBase::createVariableAccess (TConfig::globalRuntimeDataPtr, block)}, block, false);
#endif            
    } else if (lexer.checkToken (TToken::BracketOpen)) {
        expr = TExpression::parse (block);
        compiler.checkToken (TToken::BracketClose, "Bracketed expression missing ')'");
    } else if (lexer.checkToken (TToken::SquareBracketOpen))
        expr = parseSetExpression (block);
    else if (lexer.checkToken (TToken::AddrOp)) {
        expr = parseAddressOperator (block);
    } else 
        compiler.errorMessage (TCompilerImpl::SyntaxError, "Missing part in expression");
    return expr;
}

TExpressionBase *TFactor::parseIdentifier (TBlock &block) {
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    const std::string identifier = lexer.getIdentifier ();
    lexer.getNextToken ();
    
    TExpressionBase *expr = block.searchActiveRecords (identifier);
    if (!expr) {
        if (TSymbol *symbol = block.getSymbols ().searchSymbol (identifier))
            if (symbol->checkSymbolFlag (TSymbol::Constant)) {
                expr = compiler.createMemoryPoolObject<TConstantValue> (symbol);
                if (symbol->getType ()->isSet ()) {
#ifdef CREATE_9900              
                    expr = createRuntimeCall ("__copy_set_const", symbol->getType (), {expr}, block, false);
#endif
                } else if (symbol->getType () == &stdType.String) {
#ifndef CREATE_9900                
                    expr = createRuntimeCall ( "__str_make", symbol->getType (), {expr, TExpressionBase::createVariableAccess (TConfig::globalRuntimeDataPtr, block)}, block, false);
#endif
                }
            } else if (symbol->checkSymbolFlag (TSymbol::Routine))
                expr = compiler.createMemoryPoolObject<TRoutineValue> (identifier, block);
            else if (symbol->getType () && symbol->getType ()->isReference ())
                expr = compiler.createMemoryPoolObject<TReferenceVariable> (symbol, block);
            else if (symbol->checkSymbolFlag (TSymbol::Variable) || symbol->checkSymbolFlag (TSymbol::Parameter) || symbol->checkSymbolFlag (TSymbol::Alias) || symbol->checkSymbolFlag (TSymbol::Absolute))
                expr = compiler.createMemoryPoolObject<TVariable> (symbol, block);
            else if (symbol->checkSymbolFlag (TSymbol::NamedType))
                expr = parseTypeConversion (symbol, block);
            else
                compiler.errorMessage (TCompilerImpl::IdentifierExpected, "Identifier '" + identifier + "' cannot be used in expression");
        else
            expr = TPredefinedRoutine::parse (identifier, block);
    }

    bool done = false;    
    while (!done) 
        if (lexer.checkToken (TToken::SquareBracketOpen))
            expr = parseIndex (expr, block);
        else if (lexer.checkToken (TToken::Point))
            expr = parseRecordComponent (expr, block);
        else if (lexer.checkToken (TToken::Dereference))
            expr = parsePointerDereference (expr, block);
        else if (lexer.checkToken (TToken::BracketOpen))
            expr = parseFunctionCall (expr, block);
        else 
            done = true;
            
    if (expr && (expr->isLValue ()))
//                ||  (expr->isFunctionCall () && static_cast<TFunctionCall *> (expr)->getFunctionReturnTemp ())))
        expr = compiler.createMemoryPoolObject<TLValueDereference> (expr);
     return expr;
}

TExpressionBase *TFactor::parseTypeConversion (TSymbol *symbol, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    
    compiler.checkToken (TToken::BracketOpen, "'(' expected in type cast");
    TExpressionBase *expr = TExpression::parse (block);
    compiler.checkToken (TToken::BracketClose, "')' expected in type case");
    // !!!! TODO some checks
    if (expr) {
        expr->setType (symbol->getType ());
        if (expr->isLValueDereference ()) {
            TExpressionBase *base = static_cast<TLValueDereference *> (expr)->getLValue ();
            if (base->isSymbol ())
                static_cast<TVariable *> (base)->getSymbol ()->setAliased ();
        }
    }
    return expr;
}

TExpressionBase *TFactor::parseFunctionCall (TExpressionBase *function, TBlock &block) {
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    std::vector<TExpressionBase *> args;
    bool success = true;
    if (!lexer.checkToken (TToken::BracketClose)) {
        do
            if (TExpressionBase *expression = TExpression::parse (block))
                args.push_back (expression);
            else
                success = false;
        while (lexer.checkToken (TToken::Comma));
        compiler.checkToken (TToken::BracketClose, "Function call missing closing ')'");
    }

    if (function) {        
    
        if (function->isRoutine () && function->getType () == &stdType.UnresOverload)
            static_cast<TRoutineValue *> (function)->resolveCall (args, block);
        
        TType *type = function->getType ();
        if (!type->isRoutine ())
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Calling non-subroutine");
        else if (success) {
        
        // !!!! TODO: Das auch fuer parameterluse Funktionen ? Deref in TFunctionCall-Konstruktor !?
        
            if (function->isLValue ())
                function = compiler.createMemoryPoolObject<TLValueDereference> (function);
            return compiler.createMemoryPoolObject<TFunctionCall> (function, std::move (args), block, true);
        }
    }
    return nullptr;
}

TExpressionBase *TFactor::parseIndex (TExpressionBase *base, TBlock &block) {
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    createFunctionCall (base, block, true);
    do {
        TExpressionBase *index = TExpression::parse (block);
        if (base && index) {
            convertBaseType (base, block);
            TType *baseType = base->getType (), 
                  *requiredIndexType = nullptr, 
                  *resultType = nullptr;
            if (baseType->isVector ()) {
                TType *indexType = convertBaseType (index, block);
                if (indexType == &stdType.Int64) {
                    if (base->isLValue ()) 
                        base = compiler.createMemoryPoolObject<TVectorIndex> (base, index, baseType->getBaseType (), TVectorIndex::TIndexKind::Int, block);
                    else {
                        compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "L-Value required to apply integer index to vector");
                        base = nullptr;
                    }
                } else {
                    if (base->isLValue ())
                        base = compiler.createMemoryPoolObject<TLValueDereference> (base);
                    indexType = getVectorBaseType (index, block);
                    if (indexType == &stdType.Int64 || indexType == &stdType.Boolean) {
                        TVectorIndex::TIndexKind indexKind = indexType == &stdType.Int64 ? TVectorIndex::TIndexKind::IntVec : TVectorIndex::TIndexKind::BoolVec;
                        base = compiler.createMemoryPoolObject<TVectorIndex> (base, index, baseType, indexKind, block);
                    } else {
                        compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Cannot index vector with type " + index->getType ()->getName ());
                        base = nullptr;
                    }
                }
            } else {     
                if (baseType->isArray ()) {
                    requiredIndexType = static_cast<TArrayType *> (baseType)->getIndexType ();
                    resultType = baseType->getBaseType ();
                } else if (baseType->isPointer ()) {
                    requiredIndexType = &stdType.Int64;
                    resultType = baseType->getBaseType ();
                } else if (baseType == &stdType.String && base->isLValue ()) {
                    requiredIndexType = &stdType.Int64;
                    resultType = &stdType.Char;
                } else
                    compiler.errorMessage (TCompilerImpl::ArrayExpected, "Array, string or pointer expected");

                if (requiredIndexType) {
                    performTypeConversion (requiredIndexType, index, block);
                    base = compiler.createMemoryPoolObject<TArrayIndex> (base, index, resultType);
                } else
                    base = nullptr;
            }
        }
    } while (lexer.checkToken (TToken::Comma));
    compiler.checkToken (TToken::SquareBracketClose, "Array index missing closing ']'");
    return base;
}

TExpressionBase *TFactor::parseRecordComponent (TExpressionBase *base, TBlock &block) {
    TLexer &lexer = block.getLexer ();
    TCompilerImpl &compiler = block.getCompiler ();
    
    if (lexer.getToken () != TToken::Identifier) {
        compiler.errorMessage (TCompilerImpl::IdentifierExpected);
        return nullptr;
    }
    
    createFunctionCall (base, block, true);
    const std::string component = lexer.getIdentifier ();
    lexer.getNextToken ();
    if (base)
        if (TType *type = base->getType ()) {
            if (type->isRecord ()) 
                if (const TSymbol *symbol = static_cast<TRecordType *> (type)->searchComponent (component))
                    return compiler.createMemoryPoolObject<TRecordComponent> (base, component, symbol);
                else 
                    compiler.errorMessage (TCompilerImpl::IncompatibleTypes, component + " is not a component of the record");
            else 
                compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Applying selection to non-record type");
        }
    return nullptr;
}

TExpressionBase *TFactor::parsePointerDereference (TExpressionBase *base, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();

    createFunctionCall (base, block, true);
    if (base)
        if (TType *type = base->getType ()) {
            if (type->isPointer ())
                return compiler.createMemoryPoolObject<TPointerDereference> (base);
            else
                compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Pointer dereference operator applied to non-pointer type");
        }
    return nullptr;
}

TExpressionBase *TFactor::parseSetExpression (TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    TLexer &lexer = block.getLexer ();
    TEnumeratedType *baseType = nullptr;
    std::vector<TExpressionBase *> values;
    std::vector<std::pair<TExpressionBase *, TExpressionBase *>> valuePairs;
    
    bool firstExpression = true;
    if (!lexer.checkToken (TToken::SquareBracketClose)) {
        do {
            TExpressionBase *expr1 = TExpression::parse (block), *expr2 = nullptr;
            if (lexer.checkToken (TToken::Points))
                expr2 = TExpression::parse (block);
            if (expr1) 
                convertBaseType (expr1, block);
            if (expr2)
                convertBaseType (expr2, block);
                
            if (firstExpression) {
                if (expr1) {
                    TType *type = expr1->getType ();
                    if (!type->isEnumerated ())
                        compiler.errorMessage (TCompilerImpl::InvalidType, "Need enumerated type to construct set, but got '" + type->getName () + "'");
                    else                        
                        baseType = static_cast<TEnumeratedType *> (type);
                }
                firstExpression = false;
            }
            
            if (baseType && expr1) {
                if (expr1->getType () != baseType)
                    compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot include value of type '" + expr1->getType ()->getName () + "' in set of type '" + baseType->getName () + "'");
                if (expr2 && expr2->getType () != baseType)
                    compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot include value of type '" + expr2->getType ()->getName () + "' in set of type '" + baseType->getName () + "'");
                if (expr2) 
                    valuePairs.push_back ({expr1, expr2});
                else
                    values.push_back (expr1);
            }
        } while (lexer.checkToken (TToken::Comma));
        compiler.checkToken (TToken::SquareBracketClose, "']' expected to end set constructor");          
    }

    TType *resultType = compiler.createMemoryPoolObject<TSetType> (baseType);
    TSimpleConstant *constPart = compiler.createMemoryPoolObject<TSimpleConstant> ();
    constPart->setType (resultType);
    
    TExpressionBase *setConstructor = compiler.createMemoryPoolObject<TConstantValue> (constPart);
#ifdef CREATE_9900  
    setConstructor =  createRuntimeCall ("__copy_set_const", resultType, {setConstructor}, block, false);
#endif    
    for (TExpressionBase *val: values) 
        if (val->isConstant ())
            constPart->addSetValue (static_cast<TConstantValue *> (val)->getConstant ()->getInteger ());
        else 
            setConstructor = createRuntimeCall ("__set_add_val", nullptr, {compiler.createMemoryPoolObject<TTypeCast> (&stdType.Int64, val), setConstructor}, block, false);
    for (const std::pair<TExpressionBase *, TExpressionBase *> &val: valuePairs)
        if (val.first->isConstant () && val.second->isConstant ())
            constPart->addSetRange (static_cast<TConstantValue *> (val.first)->getConstant ()->getInteger (), static_cast<TConstantValue *> (val.second)->getConstant ()->getInteger ());
        else
            setConstructor = createRuntimeCall ("__set_add_range", nullptr, {compiler.createMemoryPoolObject<TTypeCast> (&stdType.Int64, val.first), compiler.createMemoryPoolObject<TTypeCast> (&stdType.Int64, val.second), setConstructor}, block, false);
    setConstructor->setType (resultType);
    return setConstructor;
}

TExpressionBase *TFactor::parseAddressOperator (TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    
    TExpressionBase *expr = TFactor::parse (block);
    if (expr) {
        TType *type = expr->getType ();
        if (type) {
            expr->setType (&stdType.GenericPointer);
            if (!type->isRoutine ()) {
                if (expr->isLValueDereference ()) {
                    expr = static_cast<TLValueDereference *> (expr)->getLValue ();
                    if (expr->isSymbol ())
                        static_cast<TVariable *> (expr)->getSymbol ()->setAliased ();
                } else 
                    compiler.errorMessage (TCompilerImpl::VariableExpected, "L-value required for address operator '@'");
            }
        }
    }
    return expr;
}


TFunctionCall::TFunctionCall (TExpressionBase *function, std::vector<TExpressionBase *> &&args_para, TBlock &block, bool checkParameter):
  function (function), args (std::move (args_para)), ignoreReturn (false), returnSymbol (nullptr), returnStorage (nullptr) {
    TCompilerImpl &compiler = block.getCompiler ();
    
    TRoutineType *routineType = static_cast<TRoutineType *> (function->getType ());
    TType *returnType = routineType->getReturnType ();
    setType (returnType);
    
    if (checkParameter) {    
        if (routineType->getParameter ().size () != args.size ()) 
            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Number of arguments does not match declaration (expected " + 
                    std::to_string (routineType->getParameter ().size ()) + " but got " + std::to_string (args.size ()) + ")");
        else {
            std::vector<TExpressionBase *>::iterator it = args.begin ();
            for (TSymbol *parameter: routineType->getParameter ()) {
                TType *formalParameterType = parameter->getType ();
                if (formalParameterType->isReference ()) {
                    if ((*it)->isLValueDereference ()) {
                        *it = static_cast<TLValueDereference *> (*it)->getLValue ();
                        formalParameterType = formalParameterType->getBaseType ();
                        if (formalParameterType != (*it)->getType () && formalParameterType != &stdType.GenericVar && !(formalParameterType == &stdType.GenericPointer && (*it)->getType ()->isPointer ()))
                            compiler.errorMessage (TCompilerImpl::IncompatibleTypes, "Expected reference of type '" + formalParameterType->getName () + "' for formal parameter " + parameter->getName () + ", but got '" + (*it)->getType ()->getName () + "'");
                    } else 
                        compiler.errorMessage (TCompilerImpl::IdentifierExpected, "L-value required for reference parameter");
                } else
                    performTypeConversion (formalParameterType, *it, block);
                ++it;
            }
        }
    }

    if (returnType != &stdType.Void && compiler.getCodeGenerator ().classifyReturnType (returnType) == TCodeGenerator::TReturnLocation::Reference) {
        static std::size_t callCount = 0;
        TSymbolList::TAddSymbolResult result = block.getSymbols ().addVariable ("$rettmp_" +  std::to_string (callCount++), routineType->getReturnType ());
        returnSymbol = result.symbol;
        returnStorage = compiler.createMemoryPoolObject<TVariable> (returnSymbol, block);
        returnStorageDeref = compiler.createMemoryPoolObject<TLValueDereference> (returnStorage);
    }
}

bool TFunctionCall::isFunctionCall () const {
    return true;
}

void TFunctionCall::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

void TFunctionCall::setReturnStorage (TExpressionBase *expr, TBlock &block) {
    returnStorage = expr;
    returnStorageDeref = block.getCompiler ().createMemoryPoolObject<TLValueDereference> (returnStorage);
    block.getSymbols ().removeSymbol (returnSymbol);
    ignoreReturn = true;
    returnSymbol = nullptr;
}


TVariable::TVariable (TSymbol *symbol, TBlock &block):
  symbol (symbol) {
    setType (symbol->getType ());
    if (symbol->getLevel () != block.getSymbols ().getLevel () && symbol->getLevel () != 1)
        block.setDisplayNeeded ();
}

//TExpressionBase::TFlags TVariable::getExpressionFlags () const {
//    return static_cast<TFlags> (LValue | Symbol);
//

bool TVariable::isLValue () const {
    return true;
}
 
bool TVariable::isSymbol () const {
    return true;
}

void TVariable::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


// TODO: brauchen wir TReferenceVariable= - Referenz steckt bereits im Typ!

TReferenceVariable::TReferenceVariable (TSymbol *symbol, TBlock &block):
  inherited (symbol, block) {
      setType (symbol->getType ()->getBaseType ());
}

bool TReferenceVariable::isReference () const {
    return true;
}

void TReferenceVariable::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TLValueDereference::TLValueDereference (TExpressionBase *base):
  base (base) {
    setType (base->getType ());
}

TExpressionBase *TLValueDereference::getLValue () const {
    // propagate possible typepast to underlying lvalue
    base->setType (getType ());
    return base;
}

bool TLValueDereference::isLValueDereference () const {
    return true;
}

void TLValueDereference::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TArrayIndex::TArrayIndex (TExpressionBase *base, TExpressionBase *index, TType *type):
  base (base), index (index) {
    setType (type);    
}

bool TArrayIndex::isLValue () const {
    return true;
}

void TArrayIndex::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TRecordComponent::TRecordComponent (TExpressionBase *base, const std::string &component, const TSymbol *symbol):
  base (base), component (component) {
    setType (symbol->getType ());
}
 
bool TRecordComponent::isLValue () const {
    return true;
}

void TRecordComponent::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TPointerDereference::TPointerDereference (TExpressionBase *expr):
  expr (expr) {
    setType (expr->getType ()->getBaseType ());
}

bool TPointerDereference::isLValue () const {
    return true;
}

void TPointerDereference::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TConstantValue::TConstantValue (const TSimpleConstant *constant):
  constant (constant), symbol (nullptr) {
    TType *type = constant->getType ();
#ifdef CREATE_9900
    setType (type == &stdType.String ? &stdType.ShortString : type);
#else    
    // TODO: check constant string expressions on x64!!!!
    setType (type == &stdType.String ? &stdType.Int64 : type);
#endif    
}

TConstantValue::TConstantValue (TSymbol *symbol):
  constant (), symbol (symbol) {
    TType *type = symbol->getType ();
#ifdef CREATE_9900
    setType (type == &stdType.String ? &stdType.ShortString : type);
#else    
    setType (type);
#endif    
}

bool TConstantValue::isConstant () const {
    return true;
}

void TConstantValue::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TRoutineValue::TRoutineValue (const std::string &identifier, TBlock &block):
  symbol (nullptr) {
    TCompilerImpl &compiler = block.getCompiler ();
    symbolOverloads = block.getSymbols ().searchSymbols (identifier);
    
    if (symbolOverloads.empty ()) {
        compiler.errorMessage (TCompilerImpl::IdentifierNotFound, "Identifier '" + identifier + "' not found in subroutine call");
        setType (&stdType.Void);
    } else {
        resolveOverload (symbolOverloads.front ());
        if (symbolOverloads.size () > 1) {
//            std::cout << "Overloaded: " << identifier << " with count: " << symbolOverloads.size () << std::endl;
            setType (&stdType.UnresOverload);
        } else {
//            std::cout << "Resolved: " << identifier << " with argscount: " << static_cast<TRoutineType *> (getType ())->getParameter ().size () << std::endl;
        }
    }
}

void TRoutineValue::resolveOverload (TSymbol *s) {
    symbol = s;
    setType (s->getType ());
}

void TRoutineValue::resolveCall (std::vector<TExpressionBase *> args, TBlock &block) {
    for (TSymbol *s: symbolOverloads) {
        TRoutineType *routineType = static_cast<TRoutineType *> (s->getType ());
        bool success = false;
        if (routineType->getParameter ().size () == args.size ()) {
            success = true;
            std::vector<TExpressionBase *>::iterator it = args.begin ();
            for (TSymbol *parameter: routineType->getParameter ()) {
                TType *formalParameterType = parameter->getType ();
                if (formalParameterType->isReference ()) {
                    formalParameterType = formalParameterType->getBaseType ();
                    if (!(*it)->isLValueDereference () ||
                        (formalParameterType != &stdType.GenericVar && 
                         formalParameterType != (*it)->getType ()))
                        success = false;
                } else if (!checkTypeConversion (formalParameterType, *it, block))
                    success = false;
                ++it;
            }
        }
        if (success) {
            resolveOverload (s);
            return;
        }
    }        
}

bool TRoutineValue::resolveConversion (const TRoutineType *required) {
    for (TSymbol *s: symbolOverloads) {
        TRoutineType *routineType = static_cast<TRoutineType *> (s->getType ());
        if (routineType->matchesOverload (required)) {
            resolveOverload (s);
            return true;
        }
    }
    return false;
}

bool TRoutineValue::isRoutine () const {
    return true;
}

void TRoutineValue::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}


TVectorIndex::TVectorIndex (TExpressionBase *base, TExpressionBase *index, TType *resultType, TIndexKind indexKind, TBlock &block) {
    static const std::map<TIndexKind, std::string> runtimeFunc = {
        {TIndexKind::IntVec, "__vec_index_vint"}, {TIndexKind::BoolVec, "__vec_index_vbool"}, {TIndexKind::Int, "__vec_index_int"}
    };
    setType (resultType);
    lValue = (indexKind == TIndexKind::Int);
    runtimeCall = createRuntimeCall (runtimeFunc.at (indexKind), lValue ? static_cast<TType *> (&stdType.GenericPointer) : resultType, {base, index}, block, false);
}

bool TVectorIndex::isLValue () const {
    return lValue;
}

bool TVectorIndex::isVectorIndex () const {
    return true;
}

void TVectorIndex::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*runtimeCall);
}

}
