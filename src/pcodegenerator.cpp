#include "pcodegenerator.hpp"

namespace statpascal {

TPCodeGenerator::TPCodeGenerator (TPCodeMemory &pcodeMemory, bool codeRangeCheck, bool createCompilerListing):
  inherited (pcodeMemory.getRuntimeData ()),
  pcodeMemory (pcodeMemory),
  predefinedFunctionMap ({
      {TPredefinedRoutine::Chr, TPCode::TOperation::NoOperation},
      {TPredefinedRoutine::Ord, TPCode::TOperation::NoOperation},
      
      {TPredefinedRoutine::Dispose, TPCode::TOperation::Free},
      
      {TPredefinedRoutine::Break, TPCode::TOperation::Break},
      {TPredefinedRoutine::Halt, TPCode::TOperation::Halt},
      
      {TPredefinedRoutine::SizeVector, TPCode::TOperation::VecLength},
      {TPredefinedRoutine::RevVector, TPCode::TOperation::VecRev}
  }),
  currentLevel (0),
  codeRangeCheck (codeRangeCheck),
  createCompilerListing (createCompilerListing) {
    buildOperationMap (); 	// we can do static init now
}

TPCodeGenerator::~TPCodeGenerator () {
}

void TPCodeGenerator::buildOperationMap () {
    operationMap = TOperationMap {
      // TExpression  
        {{&stdType.Int64, TToken::Equal}, TPCode::TOperation::EqualInt},
        {{&stdType.Int64, TToken::GreaterThan}, TPCode::TOperation::GreaterInt},
        {{&stdType.Int64, TToken::LessThan}, TPCode::TOperation::LessInt},
        {{&stdType.Int64, TToken::GreaterEqual}, TPCode::TOperation::GreaterEqualInt},
        {{&stdType.Int64, TToken::LessEqual}, TPCode::TOperation::LessEqualInt},
        {{&stdType.Int64, TToken::NotEqual}, TPCode::TOperation::NotEqualInt},

        {{&stdType.GenericVector, TToken::Equal}, TPCode::TOperation::EqualVec},
        {{&stdType.GenericVector, TToken::GreaterThan}, TPCode::TOperation::GreaterVec},
        {{&stdType.GenericVector, TToken::LessThan}, TPCode::TOperation::LessVec},
        {{&stdType.GenericVector, TToken::GreaterEqual}, TPCode::TOperation::GreaterEqualVec},
        {{&stdType.GenericVector, TToken::LessEqual}, TPCode::TOperation::LessEqualVec},
        {{&stdType.GenericVector, TToken::NotEqual}, TPCode::TOperation::NotEqualVec},
        
        {{&stdType.Real, TToken::Equal}, TPCode::TOperation::EqualDouble},
        {{&stdType.Real, TToken::GreaterThan}, TPCode::TOperation::GreaterDouble},
        {{&stdType.Real, TToken::LessThan}, TPCode::TOperation::LessDouble},
        {{&stdType.Real, TToken::GreaterEqual}, TPCode::TOperation::GreaterEqualDouble},
        {{&stdType.Real, TToken::LessEqual}, TPCode::TOperation::LessEqualDouble},
        {{&stdType.Real, TToken::NotEqual}, TPCode::TOperation::NotEqualDouble},
        // TSimpleExpression
        {{&stdType.Boolean, TToken::Or}, TPCode::TOperation::OrInt},
        {{&stdType.Boolean, TToken::Xor}, TPCode::TOperation::XorInt},
        
        {{&stdType.Int64, TToken::Add}, TPCode::TOperation::AddInt},
        {{&stdType.Int64, TToken::Sub}, TPCode::TOperation::SubInt},
        {{&stdType.Int64, TToken::Or}, TPCode::TOperation::OrInt},
        {{&stdType.Int64, TToken::Xor}, TPCode::TOperation::XorInt},
        
        {{&stdType.GenericVector, TToken::Add}, TPCode::TOperation::AddVec},
        {{&stdType.GenericVector, TToken::Sub}, TPCode::TOperation::SubVec},
        {{&stdType.GenericVector, TToken::Or}, TPCode::TOperation::OrVec},
        {{&stdType.GenericVector, TToken::Xor}, TPCode::TOperation::XorVec},
        
        {{&stdType.Real, TToken::Add}, TPCode::TOperation::AddDouble},
        {{&stdType.Real, TToken::Sub}, TPCode::TOperation::SubDouble},
        
        // TTerm
        {{&stdType.Boolean, TToken::And}, TPCode::TOperation::AndInt},
        
        {{&stdType.Int64, TToken::Mul}, TPCode::TOperation::MulInt},
        {{&stdType.Int64, TToken::DivInt}, TPCode::TOperation::DivInt},
        {{&stdType.Int64, TToken::Mod}, TPCode::TOperation::ModInt},
        {{&stdType.Int64, TToken::And}, TPCode::TOperation::AndInt},
        {{&stdType.Int64, TToken::Shl}, TPCode::TOperation::ShlInt},
        {{&stdType.Int64, TToken::Shr}, TPCode::TOperation::ShrInt},
        
        {{&stdType.GenericVector, TToken::Mul}, TPCode::TOperation::MulVec},
        {{&stdType.GenericVector, TToken::DivInt}, TPCode::TOperation::DivIntVec},
        {{&stdType.GenericVector, TToken::Div}, TPCode::TOperation::DivVec},
        {{&stdType.GenericVector, TToken::Mod}, TPCode::TOperation::ModVec},
        {{&stdType.GenericVector, TToken::And}, TPCode::TOperation::AndVec},
        
        {{&stdType.Real, TToken::Mul}, TPCode::TOperation::MulDouble},
        {{&stdType.Real, TToken::Div}, TPCode::TOperation::DivDouble},
        
        // TPrefixedExpression
        {{&stdType.Int64, TToken::Not}, TPCode::TOperation::NotInt},
        {{&stdType.Boolean, TToken::Not}, TPCode::TOperation::NotBool}
    };
    memoryOperationMap = TMemoryOperationMap {
        {&stdType.Uint8,          {TPCode::TOperation::LoadUint8,  TPCode::TOperation::Store8,      TPCode::TOperation::Push8      }},
        {&stdType.Int8,           {TPCode::TOperation::LoadInt8,   TPCode::TOperation::Store8,      TPCode::TOperation::Push8      }},
        {&stdType.Uint16,         {TPCode::TOperation::LoadUint16, TPCode::TOperation::Store16,     TPCode::TOperation::Push16     }},
        {&stdType.Int16,          {TPCode::TOperation::LoadInt16,  TPCode::TOperation::Store16,     TPCode::TOperation::Push16     }},
        {&stdType.Uint32,         {TPCode::TOperation::LoadUint32, TPCode::TOperation::Store32,     TPCode::TOperation::Push32     }},
        {&stdType.Int32,          {TPCode::TOperation::LoadInt32,  TPCode::TOperation::Store32,     TPCode::TOperation::Push32     }},
        {&stdType.Int64,          {TPCode::TOperation::LoadInt64,  TPCode::TOperation::Store64,     TPCode::TOperation::Push64     }},
        {&stdType.Real,           {TPCode::TOperation::LoadDouble, TPCode::TOperation::StoreDouble, TPCode::TOperation::PushDouble }},
        {&stdType.Single,         {TPCode::TOperation::LoadSingle, TPCode::TOperation::StoreSingle, TPCode::TOperation::PushSingle }},
        {&stdType.GenericVector,  {TPCode::TOperation::LoadVec,    TPCode::TOperation::StoreVec,    TPCode::TOperation::PushVec    }},
        {&stdType.GenericPointer, {TPCode::TOperation::LoadPtr,    TPCode::TOperation::StorePtr,    TPCode::TOperation::PushPtr    }}
    };
}

const std::vector<std::string> &TPCodeGenerator::getCompilerListing () {
    if (createCompilerListing && compilerListing.empty ())
        createCodeListing ();
    return compilerListing;
}

void TPCodeGenerator::outputBooleanShortcut (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    visit (left);
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, operation == TToken::Or, 0));
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::EqualInt));
    
    std::size_t fixup1 = pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::JumpZero));
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, operation != TToken::And, 0));
    std::size_t fixup2 = pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Jump));
    
    pcodeMemory.outputPCode (fixup1, TPCodeOperation (TPCode::TOperation::JumpZero, pcodeMemory.getOutputPosition (), 0));
    visit (right);
    pcodeMemory.outputPCode (fixup2, TPCodeOperation (TPCode::TOperation::Jump, pcodeMemory.getOutputPosition (), 0));
}

void TPCodeGenerator::outputBinaryOperation (TToken operation, TExpressionBase *left, TExpressionBase *right) {
    TType *type = left->getType ();
    if (type == &stdType.Boolean && (operation == TToken::And || operation == TToken::Or)) 
        outputBooleanShortcut (operation, left, right);
    else {
        visit (left);
        visit (right);
        
        if (type->isPointer () && (operation == TToken::Add || operation == TToken::Sub)) {
            std::size_t basesize = std::max<std::size_t> (type->getBaseType ()->getSize (), 1);
            if (operation == TToken::Add) 
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CalcIndex, 0,  basesize));
            else {
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::SubInt));
                if (basesize > 1) {
                    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, basesize, 0));
                    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::DivInt));
                }
            }
        } else {
            if (type->isPointer () || type->isEnumerated ())
                type = &stdType.Int64;
            else if (type->isVector ()) {
                TType *baseType = type->getBaseType ();
                if (baseType->isPointer () || baseType->isEnumerated () || baseType->isReal () || baseType == &stdType.Single)
                    type = &stdType.GenericVector;
            }
            
            const TOperationMap::const_iterator it = operationMap.find (std::make_pair (type, operation));
            if (it != operationMap.end ())
                if (type == &stdType.GenericVector) {
                    TType *leftBase = static_cast<TVectorType *> (left->getType ())->getBaseType (),
                          *rightBase = static_cast<TVectorType *> (right->getType ())->getBaseType ();
                    pcodeMemory.outputPCode (TPCodeOperation (it->second, static_cast<std::size_t> (getScalarTypeCode (leftBase)), static_cast<std::size_t> (getScalarTypeCode (rightBase))));
                } else
                    pcodeMemory.outputPCode (TPCodeOperation (it->second));
            else {
                std::cout << "Internal error: Operation " << operation << " not found for " << left->getType ()->getName () << std::endl;
                exit (1);
            }
        }
    }
}

void TPCodeGenerator::generateCode (TTypeCast &typeCast) {
    TExpressionBase *expression = typeCast.getExpression ();
    TType *destType = typeCast.getType (),
          *srcType = expression->getType ();
    
    visit (expression);
    if (srcType == &stdType.Int64 && destType == &stdType.Real) {
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::ConvertIntDouble));
        return;
    }
    if (srcType == &stdType.String && destType->isPointer ()) {
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::StrPtr));
        return;
    }
    
    if (destType->isEnumerated ()) {
        const TEnumeratedType *srcEnumeratedType = static_cast<const TEnumeratedType *> (srcType),
                              *destEnumeratedType = static_cast<const TEnumeratedType *> (destType);
        if (srcEnumeratedType->getMinVal () < destEnumeratedType->getMinVal () || srcEnumeratedType->getMaxVal () > destEnumeratedType->getMaxVal ())
            generateRangeCheck (destEnumeratedType->getMinVal (), destEnumeratedType->getMaxVal ());
    }

    if (destType->isVector () && destType->getBaseType () == srcType) {
        TTypeAnyManager typeAnyManager = lookupAnyManager (srcType);
        TPCode::TOperation pcode = lookupMemoryOperation (srcType).found ? TPCode::TOperation::MakeVec : TPCode::TOperation::MakeVecMem;
        pcodeMemory.outputPCode (TPCodeOperation (pcode, srcType->getSize (), typeAnyManager.runtimeIndex));
        return;
    }
    if (destType->isVector () && srcType->isVector ()) {
        const TType *srcBaseType = srcType->getBaseType (),
                    *destBaseType = destType->getBaseType ();
        TPCode::TScalarTypeCode destTypeCode = getScalarTypeCode (destBaseType),
                                srcTypeCode = getScalarTypeCode (srcBaseType);
        if (destTypeCode != TPCode::TScalarTypeCode::invalid && srcTypeCode != TPCode::TScalarTypeCode::invalid) {
            if (destBaseType->isEnumerated ()) {
                const TEnumeratedType *srcEnumeratedType = static_cast<const TEnumeratedType *> (srcBaseType),
                                      *destEnumeratedType = static_cast<const TEnumeratedType *> (destBaseType);
                if (srcEnumeratedType->getMinVal () < destEnumeratedType->getMinVal () || srcEnumeratedType->getMaxVal () > destEnumeratedType->getMaxVal ())
                    generateRangeCheck (destEnumeratedType->getMinVal (), destEnumeratedType->getMaxVal (), true, srcTypeCode);
            }
            if (destTypeCode != srcTypeCode) 
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::ConvertVec, static_cast<std::size_t> (srcTypeCode), static_cast<std::size_t> (destTypeCode)));
        }
    }
}

void TPCodeGenerator::generateCode (TExpression &comparison) {
    outputBinaryOperation (comparison.getOperation (), comparison.getLeftExpression (), comparison.getRightExpression ());
}

void TPCodeGenerator::generateCode (TPrefixedExpression &prefixed) {
    visit (prefixed.getExpression ());
    const TOperationMap::const_iterator it = operationMap.find (std::make_pair (prefixed.getExpression ()->getType (), prefixed.getOperation ()));
    if (it != operationMap.end ())
        // TToken::Sub is unary here
        if (it->second == TPCode::TOperation::SubInt)
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::NegInt));
        else if (it->second == TPCode::TOperation::SubDouble)
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::NegDouble));
        else
            pcodeMemory.outputPCode (TPCodeOperation (it->second));
    else {
        std::cout << "Internal error: Unary peration " << prefixed.getOperation () << " not found for " << prefixed.getExpression ()->getType ()->getName () << std::endl;
        exit (1);
    }
}

void TPCodeGenerator::generateCode (TSimpleExpression &expression) {
    outputBinaryOperation (expression.getOperation (), expression.getLeftExpression (), expression.getRightExpression ());
}

void TPCodeGenerator::generateCode (TTerm &term) {
    outputBinaryOperation (term.getOperation (), term.getLeftExpression (), term.getRightExpression ());
}

void TPCodeGenerator::generateCode (TVectorIndex &vectorIndex) {
    visit (vectorIndex.getBaseExpression ());
    visit (vectorIndex.getIndexExpression ());
    const TType *indexType = vectorIndex.getIndexExpression ()->getType ();
    switch (vectorIndex.getIndexKind ()) {
        case TVectorIndex::TIndexKind::IntVec:
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::VecIndexIntVec, 
                static_cast<std::size_t> (getScalarTypeCode (static_cast<const TEnumeratedType *> (indexType->getBaseType ()))), 0));
            break;
        case TVectorIndex::TIndexKind::BoolVec:
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::VecIndexBoolVec));
            break;
        case TVectorIndex::TIndexKind::Int:
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::VecIndexInt));
            break;
    }
}

void TPCodeGenerator::generateCode (TCombineRoutine &combineRoutine) {
    const std::vector<TExpressionBase *> &arguments = combineRoutine.getArguments ();
    TType *destBaseType = combineRoutine.getType ()->getBaseType ();
    for (std::vector<TExpressionBase *>::const_reverse_iterator it = arguments.rbegin (); it != arguments.rend (); ++it) {
        visit (*it);
        const TType *type = (*it)->getType ();
        TVectorCombineOption option = CombineLValue;
        if (type == &stdType.Single)
            option = CombineSingle;
        if (type == &stdType.String)
            option = CombineString;
        else if (type->isEnumerated () || type->isReal ())
            option = CombineScalar;
        else if (type->isVector ()) {
            const TType *srcBaseType = type->getBaseType ();
            if (srcBaseType == destBaseType)
                option = CombineVector;
            else
                option = CombineString;
        }
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, option, 0));
    }
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, arguments.size (), 0));
    TTypeAnyManager typeAnyManager = lookupAnyManager (destBaseType);
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CombineVec, destBaseType->getSize (), typeAnyManager.runtimeIndex));
}

void TPCodeGenerator::generateCode (TFunctionCall &functionCall) {
    TExpressionBase *function = functionCall.getFunction ();
    const std::vector<TExpressionBase *> &args = functionCall.getArguments ();
    
    TExpressionBase *returnTempStorage = functionCall.getReturnTempStorage (),
                    *returnStorage = functionCall.getReturnStorage (),
                    *loadReturnAddress = returnStorage;
    if (!loadReturnAddress) {
        loadReturnAddress = returnTempStorage;
        if (loadReturnAddress)
            loadReturnAddress = static_cast<TLValueDereference *> (loadReturnAddress)->getLValue ();
    } 
    if (loadReturnAddress) {
        visit (loadReturnAddress);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::PushPtr));
    }
        
    for (std::vector<TExpressionBase *>::const_reverse_iterator it = args.rbegin (); it != args.rend (); ++it)
        visit (*it);
    // Parameters are collected on calculator stack to keep stack alignment correct during evaluation
    std::vector<TExpressionBase *>::const_iterator it = args.begin ();
    for (const TSymbol *s: static_cast<TRoutineType *> (function->getType ())->getParameter ()) {
//    for (std::vector<TExpressionBase *>::const_iterator it = args.begin (); it != args.end (); ++it) {
        TType *type = s->getType ();
        TPCode::TOperation pcode = TPCode::TOperation::IllegalOperation;
        if ((*it)->isLValue () || type->isReference ())
            pcode = TPCode::TOperation::PushPtr;
        else {
            TMemoryOperationLookup memop = lookupMemoryOperation (type);
            if (memop.found)
                pcode = memop.operation.push;
        }
        if (pcode == TPCode::TOperation::IllegalOperation) {
            if (type->getAlignment () > 1)
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::AlignStack, type->getAlignment (), 0));
            TTypeAnyManager typeAnyManager = lookupAnyManager (type);
            if (typeAnyManager.anyManager)
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::PushMemAny, type->getSize (), typeAnyManager.runtimeIndex));
            else
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::PushMem, type->getSize (), 0));
        } else 
            pcodeMemory.outputPCode (TPCodeOperation (pcode));
        ++it;
    }
        
    if (function->isRoutine ()) {
        TRoutineValue &routineValue = *static_cast<TRoutineValue *> (function);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Call, getRoutinePosition (routineValue), 0));
    } else {
        visit (function);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CallStack));
    }

    if (!returnStorage && returnTempStorage && !functionCall.isIgnoreReturn ()) 
        visit (returnTempStorage);
    
}

void TPCodeGenerator::generateCode (TConstantValue &constant) {
    if (constant.getType () == &stdType.Real)
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadDoubleConst, constant.getConstant ()->getDouble ()));
    else if (constant.getConstant ()->getType () == &stdType.String) {
        std::size_t stringIndex = pcodeMemory.getRuntimeData ().registerStringConstant (constant.getConstant ()->getString ());
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, stringIndex, 0));
    } else if (constant.getType ()->isSet ()) {
        std::size_t blobIndex = pcodeMemory.getRuntimeData ().registerData (&constant.getConstant ()->getSetValues () [0], sizeof (std::int64_t) * TConfig::setwords);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, reinterpret_cast<std::uint64_t> (pcodeMemory.getRuntimeData ().getData (blobIndex)), 0));        
    } else
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, constant.getConstant ()->getInteger (), 0));
}

void TPCodeGenerator::generateCode (TRoutineValue &routineValue) {
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, getRoutinePosition (routineValue), 0));
}

void TPCodeGenerator::generateCode (TVariable &variable) {
    const TSymbol *symbol = variable.getSymbol ();
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadAddress, symbol->getLevel (), symbol->getOffset ()));
}

void TPCodeGenerator::generateCode (TReferenceVariable &referenceVariable) {    
    const TSymbol *symbol = referenceVariable.getSymbol ();
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadAddress, symbol->getLevel (), symbol->getOffset ()));
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadPtr));
}

void TPCodeGenerator::generateCode (TLValueDereference &lValueDereference) {
    TMemoryOperationLookup memop = lookupMemoryOperation (lValueDereference.getType ());
    // for a non-scalar, a pointer is left on the stack
    visit (lValueDereference.getLValue ());
    if (memop.found)
        pcodeMemory.outputPCode (memop.operation.load);
}

void TPCodeGenerator::generateCode (TArrayIndex &arrayIndex) {
    visit (arrayIndex.getBaseExpression ());
    
    const TType *type = arrayIndex.getBaseExpression ()->getType ();
    if (type == &stdType.String) {
        visit (arrayIndex.getIndexExpression ());
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::StrIndex));
    } else {
        ssize_t minVal = 0;
        const std::size_t baseSize = arrayIndex.getType ()->getSize ();
        if (type->isPointer ())
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadPtr));
        else 
            minVal = static_cast<TEnumeratedType *> (arrayIndex.getIndexExpression ()->getType ())->getMinVal ();
        visit (arrayIndex.getIndexExpression ());
        if (minVal || baseSize > 1)
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CalcIndex, minVal, baseSize));
        else
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::AddInt));
    }
}

void TPCodeGenerator::generateCode (TRecordComponent &recordComponent) {
    visit (recordComponent.getExpression ());
    TRecordType *recordType = static_cast<TRecordType *> (recordComponent.getExpression ()->getType ());
    if (const TSymbol *symbol = recordType->searchComponent (recordComponent.getComponent ())) {
        if (symbol->getOffset ()) {
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, symbol->getOffset (), 1));
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::AddInt));
        }
    } else {
        // INternal Error: Component not found !!!!
    }
}

void TPCodeGenerator::generateCode (TPointerDereference &pointerDereference) {
    TExpressionBase *base = pointerDereference.getExpression ();
    visit (base);
    if (base->isLValue ())
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadPtr));
}

void TPCodeGenerator::generateCode (TRuntimeRoutine &transformedRoutine) {
    for (TSyntaxTreeNode *node: transformedRoutine.getTransformedNodes ())
        visit (node);
}

void TPCodeGenerator::generateCode (TPredefinedRoutine &predefinedRoutine) {
    const std::vector<TExpressionBase *> &arguments = predefinedRoutine.getArguments ();
    if (predefinedRoutine.getRoutine () != TPredefinedRoutine::Inc &&
        predefinedRoutine.getRoutine () != TPredefinedRoutine::Dec)
        for (TExpressionBase *expression: arguments)
            visit (expression);
            
    TPredefinedFunctionMap::const_iterator it = predefinedFunctionMap.find (predefinedRoutine.getRoutine ());
    if (it != predefinedFunctionMap.end ()) {
        if (it->second != TPCode::TOperation::NoOperation)
            pcodeMemory.outputPCode (TPCodeOperation (it->second));
    } else 
        switch (predefinedRoutine.getRoutine ()) {
            case TPredefinedRoutine::Addr:
                if (TExpressionBase *expr = predefinedRoutine.getArguments () [0])
                    if (expr->isRoutine ()) 
                        if (const TSymbol *symbol = dynamic_cast<const TRoutineValue *> (expr)->getSymbol ())
                            if (symbol->checkSymbolFlag (TSymbol::Export))
                                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadExportAddr));
                break;
            case TPredefinedRoutine::Odd:
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, 1, 0));
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::AndInt));
                break;
            case TPredefinedRoutine::ReserveVector: {
                TType *basetype = predefinedRoutine.getArguments () [0]->getType ()->getBaseType ();
                TTypeAnyManager typeAnyManager = lookupAnyManager (basetype);
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::ResizeVec, basetype->getSize (), typeAnyManager.runtimeIndex));
                break; }
            case TPredefinedRoutine::Pred: 
            case TPredefinedRoutine::Succ: {
                const bool isSucc = predefinedRoutine.getRoutine () == TPredefinedRoutine::Succ;
                const TEnumeratedType *type = static_cast<const TEnumeratedType *> (predefinedRoutine.getType ());
                std::int64_t minval = type->getMinVal (), 
                             maxval = type->getMaxVal ();
                if (isSucc)
                    --maxval;
                else 
                    ++minval;
                generateRangeCheck (minval, maxval);
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::AddInt64Const, isSucc ? 1 : -1, 0));
                break; }
            case TPredefinedRoutine::Inc:
            case TPredefinedRoutine::Dec: {
                TType *type = static_cast<TEnumeratedType *> (arguments [0]->getType ());
                TMemoryOperationLookup memop = lookupMemoryOperation (type);
                if (!memop.found)
                    abort (); 	// !!!!
                visit (arguments [0]); 
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Dup));
                pcodeMemory.outputPCode (memop.operation.load);
                
                bool isInc = predefinedRoutine.getRoutine () == TPredefinedRoutine::Inc;
                std::size_t n = type->isPointer () ? static_cast<TPointerType *> (type)->getBaseType ()->getSize () : 1;
                if (arguments.size () == 1)
                    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::AddInt64Const, isInc ? n : -n, 0));
                else {
                    visit (arguments [1]);
                    if (n != 1) {
                        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, n, 0));
                        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::MulInt));
                    }
                    pcodeMemory.outputPCode (TPCodeOperation (isInc ? TPCode::TOperation::AddInt : TPCode::TOperation::SubInt));
                }
                if (type->isEnumerated ())
                    generateRangeCheck (static_cast<TEnumeratedType *> (type)->getMinVal (), static_cast<TEnumeratedType *> (type)->getMaxVal ());
                pcodeMemory.outputPCode (memop.operation.store);
                break; }
            case TPredefinedRoutine::New: {
                if (arguments.size () == 1)
                    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, 1, 0));
                TType *basetype = arguments [0]->getType ()->getBaseType ();
                TTypeAnyManager typeAnyManager = lookupAnyManager (basetype);
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Alloc, basetype->getSize (), typeAnyManager.runtimeIndex));
                break; }
            case TPredefinedRoutine::Exit:
                unresolvedExitCalls.push_back (pcodeMemory.outputPCode (TPCode::TOperation::Jump));
                break;
            default:
                std::cout << "Internal error: Cannot generate code for predefined fuction " << static_cast<unsigned> (predefinedRoutine.getRoutine ()) << std::endl;
                exit (1);
                break;
        }
}

void TPCodeGenerator::generateCode (TAssignment &assignment) {
    TExpressionBase *lValue = assignment.getLValue (),
                    *expression = assignment.getExpression ();
    TType *type = lValue->getType ();
    TMemoryOperationLookup memop = lookupMemoryOperation (type);

    visit (lValue);
    visit (expression);
    if (memop.found) {
        pcodeMemory.outputPCode (TPCodeOperation (memop.operation.store));
    } else {
        TTypeAnyManager typeAnyManager = lookupAnyManager (type);
        if (typeAnyManager.anyManager)
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CopyMemAny, type->getSize (), typeAnyManager.runtimeIndex));
        else
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CopyMem, type->getSize (), 0));
    }
}

void TPCodeGenerator::generateCode (TRoutineCall &routineCall) {
    visit (routineCall.getRoutineCall ());
}

void TPCodeGenerator::generateCode (TSimpleStatement &simpleStatement) {
/*
    TExpressionBase *leftExpression = simpleStatement.getLeftExpression ();
    TType *type = (leftExpression->getType ());
    TMemoryOperationLookup memop = lookupMemoryOperation (type);

    visit (leftExpression);
    if (TExpressionBase *rightExpression = simpleStatement.getRightExpression ()) {
        visit (rightExpression);
        if (memop.found) {
            pcodeMemory.outputPCode (TPCodeOperation (memop.operation.store));
        } else {
            TTypeAnyManager typeAnyManager = lookupAnyManager (type);
            if (typeAnyManager.anyManager)
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CopyMemAny, type->getSize (), typeAnyManager.runtimeIndex));
            else
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CopyMem, type->getSize (), 0));
        }
    }
*/    
}

void TPCodeGenerator::generateCode (TIfStatement &ifStatement) {
    visit (ifStatement.getCondition ());
    std::size_t pos1 = pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::JumpZero));
    visit (ifStatement.getStatement1 ());
    if (TStatement *statement2 = ifStatement.getStatement2 ()) {
        std::size_t pos2 = pcodeMemory.outputPCode (TPCode::TOperation::Jump); 
        pcodeMemory.outputPCode (pos1, TPCodeOperation (TPCode::TOperation::JumpZero, pcodeMemory.getOutputPosition (), 0));
        visit (statement2);
        pcodeMemory.outputPCode (pos2, TPCodeOperation (TPCode::TOperation::Jump, pcodeMemory.getOutputPosition (), 0));
    } else
         pcodeMemory.outputPCode (pos1, TPCodeOperation (TPCode::TOperation::JumpZero, pcodeMemory.getOutputPosition (), 0));
}

void TPCodeGenerator::generateCode (TCaseStatement &caseStatement) {
    visit (caseStatement.getExpression ());
    pcodeMemory.outputPCode (TPCode::TOperation::StoreAcc);
    
    std::vector<std::size_t> fixupJumpEnd;
    const TCaseStatement::TCaseList &caseList = caseStatement.getCaseList ();
    TStatement *defaultStatement = caseStatement.getDefaultStatement ();
    
    for (const TCaseStatement::TCase &c: caseList) {
        std::size_t statementPos = pcodeMemory.getOutputPosition () + 1;
        for (const TCaseStatement::TLabel &label: c.labels)
            statementPos += (1 + (label.a != label.b));
        for (const TCaseStatement::TLabel &label: c.labels) {
            if (label.a == label.b)
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::JumpAccEqual, label.a, statementPos));
            else {
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CmpAccRange, label.a, label.b));
                pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::JumpNotZero, statementPos, 0));
            }
        }
        std::size_t jumpNextPos = pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Jump, 0, 0));
        visit (c.statement);
        if (&c != &caseList.back () || defaultStatement)
            fixupJumpEnd.push_back (pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Jump, 0, 0)));
        pcodeMemory.outputPCode (jumpNextPos, TPCodeOperation (TPCode::TOperation::Jump, pcodeMemory.getOutputPosition (), 0));
    }
    if (defaultStatement)
        visit (defaultStatement);
    for (std::size_t pos: fixupJumpEnd)
        pcodeMemory.outputPCode (pos, TPCodeOperation (TPCode::TOperation::Jump, pcodeMemory.getOutputPosition (), 0));
}

void TPCodeGenerator::generateCode (TStatementSequence &statementSequence) {
    for (TStatement *statement: statementSequence.getStatements ())
        visit (statement);
}

/*
void TPCodeGenerator::generateCode (TRepeatStatement &repeatStatement) {
    std::size_t posBegin = pcodeMemory.getOutputPosition ();
    for (TStatement *statement: repeatStatement.getStatements ())
        visit (statement);
    visit (repeatStatement.getCondition ());
    pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::JumpZero, posBegin, 0));
}
*/

void TPCodeGenerator::generateCode (TLabeledStatement &labeledStatement) {
    labeledStatement.getLabel ()->setOffset (pcodeMemory.getOutputPosition ());
    visit (labeledStatement.getStatement ());
}

void TPCodeGenerator::generateCode (TGotoStatement &gotoStatement) {
    TPCode::TOperation op = TPCode::TOperation::Jump;
    if (TExpressionBase *condition = gotoStatement.getCondition ()) {
        visit (condition);
        op = TPCode::TOperation::JumpNotZero;
    }
    gotoStatement.getLabel ()->appendUnresolvedReferencePosition (pcodeMemory.getOutputPosition ());
    pcodeMemory.outputPCode (TPCodeOperation (op, 0, 0));
}

TCallbackDescription TPCodeGenerator::getCallbackDescription (const TRoutineType *routineType) {
    std::vector<TExternalType> parameter;
    for (const TSymbol *s: routineType->getParameter ())
        parameter.push_back (getExternalType (s->getType (), false));
    return TCallbackDescription (getExternalType (routineType->getReturnType (), false), parameter);
}

void TPCodeGenerator::generateCode (TUnit &unit) {
}

void TPCodeGenerator::generateCode (TBlock &block) {
    generateBlock (block);
}

void TPCodeGenerator::generateCode (TProgram &program) {
    generateBlock (*program.getBlock ());
}

TPCodeGenerator::TMemoryOperationLookup TPCodeGenerator::lookupMemoryOperation (TType *type) const {
    TMemoryOperationMap::const_iterator it = memoryOperationMap.find (type);
    if (it == memoryOperationMap.end ()) {
        if (type->isEnumerated ()) {
            const TEnumeratedType *enumType = reinterpret_cast<const TEnumeratedType *> (type);
            switch (enumType->getSize ()) {
                case 1:
                    type = enumType->isSigned () ? &stdType.Int8 : &stdType.Uint8;
                    break;
                case 2:
                    type = enumType->isSigned () ? &stdType.Int16 : &stdType.Uint16;
                    break;
                case 4:
                    type = enumType->isSigned () ? &stdType.Int32 : &stdType.Uint32;
                    break;
                case 8:
                    type = &stdType.Int64;
                    break;
            }
        } else if (type->isPointer () || type->isRoutine ())
            type = &stdType.GenericPointer;
        else if (type->isVector ())
            type = &stdType.GenericVector;
        it = memoryOperationMap.find (type);
    }
        
    if (it != memoryOperationMap.end ()) 
        return {true, it->second};
    else
        return {false};
}

TCodeGenerator::TParameterLocation TPCodeGenerator::classifyType (const TType *) {
    return TParameterLocation::Stack;
}

TCodeGenerator::TReturnLocation TPCodeGenerator::classifyReturnType (const TType *type) {
    return TReturnLocation::Reference;
}

bool TPCodeGenerator::isFunctionCallInlined (TFunctionCall &) {
    return false;
}

bool TPCodeGenerator::isReferenceCallerCopy (const TType *type) {
    return false;
}

ssize_t TPCodeGenerator::getRoutinePosition (TRoutineValue &routineValue) {
    TSymbol *symbol = routineValue.getSymbol ();
    ssize_t offset = symbol->getOffset ();
    if (!offset)
        symbol->appendUnresolvedReferencePosition (pcodeMemory.getOutputPosition ());
    return offset;
}

/*
void TPCodeGenerator::assignValue (TSymbolList &predefinedSymbols, const std::string &s, std::int64_t val) {
    if (TSymbol *symbol = predefinedSymbols.searchSymbol (s)) {
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadAddress, symbol->getLevel (), symbol->getOffset ()));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, val, 0));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Store64));
    } else {
        // TODO: Internal error
    }
}
*/

void TPCodeGenerator::beginRoutineBody (const std::string &routineName, std::size_t level, TSymbolList &symbolList) {
    currentLevel = level;
    if (level > 1 && createCompilerListing)
        outputSymbolList (routineName, level, symbolList);
    if (level > 1 && (symbolList.getLocalSize () || symbolList.getParameterSize ()))
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Enter, level, symbolList.getLocalSize () * (level > 1)));
}

TPCode::TScalarTypeCode TPCodeGenerator::getScalarTypeCode (const TType *type) const {
    if (type == &stdType.Real)
        return TPCode::TScalarTypeCode::real;
    if (type == &stdType.Single)
        return TPCode::TScalarTypeCode::single;
    if (type->isEnumerated ()) {
        const TEnumeratedType *enumeratedType = static_cast<const TEnumeratedType *> (type);
        bool isSigned = enumeratedType->isSigned ();
        switch (enumeratedType->getSize ()) {
            case 1: 
                return isSigned ? TPCode::TScalarTypeCode::s8 : TPCode::TScalarTypeCode::u8;
            case 2:
                return isSigned ? TPCode::TScalarTypeCode::s16 : TPCode::TScalarTypeCode::u16;
            case 4:
                return isSigned ? TPCode::TScalarTypeCode::s32 : TPCode::TScalarTypeCode::u32;
            case 8:
                return TPCode::TScalarTypeCode::s64;
        }
    }
    return TPCode::TScalarTypeCode::invalid;
}

void TPCodeGenerator::outputDestructors (const TSymbolList &symbolList) {
    if (TAnyManager *anyManager = buildAnyManager (symbolList)) {
        std::size_t index = pcodeMemory.getRuntimeData ().registerAnyManager (anyManager);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Destructors, index, 0));
    }
}

void TPCodeGenerator::endRoutineBody (std::size_t level, TSymbolList &symbolList) {
    outputDestructors (symbolList);
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Label))
            for (std::size_t pos: s->getUnresolvedReferencePositions ())
                pcodeMemory.fixupRoutineAddress (pos, s->getOffset ());
//                pcodeMemory.outputPCode (pos, TPCodeOperation (TPCode::TOperation::Jump, s->getOffset (), 0));
    if  (level > 1 && (symbolList.getLocalSize () || symbolList.getParameterSize ()))
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Leave, level, symbolList.getLocalSize ()));
    pcodeMemory.outputPCode (TPCodeOperation (level == 1 ? TPCode::TOperation::Stop : TPCode::TOperation::Return, symbolList.getParameterSize (), 0));
}

TExternalType TPCodeGenerator::getExternalType (const TType *type, bool isReturnValue) {
    static std::map<std::size_t, TExternalType> typemap = {
        {1, TExternalType::ExtInt8},
        {2, TExternalType::ExtInt16},
        {4, TExternalType::ExtInt32},
        {8, TExternalType::ExtInt64}
    };    
    
    if (type->isVector () || type->isString () || type->isPointer () || (type->isReference () && !isReturnValue))
        return TExternalType::ExtPointer;
    else if (type->isEnumerated ())
        return typemap [type->getSize ()];
    else if (type == &stdType.Single)
        return TExternalType::ExtSingle;
    else if (type == &stdType.Real)
        return TExternalType::ExtDouble;
    else if (type == &stdType.Void)
        return TExternalType::ExtVoid;
    else if (type->isSet ())
        return TExternalType::ExtSet;
    else {
        std::cerr << "Internal error: cannot call external function with type " << type->getName () << std::endl;
        exit (1);
    }
}

void TPCodeGenerator::externalRoutine (TSymbol &symbol) {
    TSymbolList &parameter = symbol.getBlock ()->getSymbols ();
    assignStackOffsets (parameter);
    
    const std::size_t level = symbol.getLevel () + 1;
    if (createCompilerListing)
        outputSymbolList (symbol.getName (), level, parameter);

    bool useFFI = symbol.checkSymbolFlag (TSymbol::ExternalFFI);        
    TExternalRoutine externalRoutine (symbol.getExtLibName (), symbol.getExtSymbolName (), useFFI);
    if (useFFI)
        for (TSymbol *s: parameter) {
            TType *type = s->getType ();
            bool isReturnValue = !s->getName ().empty () && s->getName ().front () == '$';
            if (isReturnValue)
                type = type->getBaseType ();
            if (type->isRoutine () && !isReturnValue)
                externalRoutine.addCallback (s->getOffset (), getCallbackDescription (static_cast<const TRoutineType *> (type)));
            else if (isReturnValue)
                externalRoutine.setResult (s->getOffset (), getExternalType (type, true));
            else
                externalRoutine.addParameter (s->getOffset (), getExternalType (type, false));
        }
    else if (!parameter.empty ())
        externalRoutine.setParameterOffset ((*parameter.begin ())->getOffset ());
    std::size_t index = pcodeMemory.registerExternalRoutine (std::move (externalRoutine));
    
    // TODO: Unify with normal routine !!!!
  
    if (useFFI) {  
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Enter, level, parameter.getLocalSize ()));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CallFFI, index, 0));
        outputDestructors (parameter);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Leave, level, parameter.getLocalSize ()));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Return, parameter.getParameterSize (), 0));
    } else {
        std::size_t anyManagerIndex = 0;
        if (TAnyManager *anyManager = buildAnyManager (parameter))
            anyManagerIndex = pcodeMemory.getRuntimeData ().registerAnyManager (anyManager);
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::CallExtern, index, anyManagerIndex));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::Return, parameter.getParameterSize (), 0));
    }
}

void TPCodeGenerator::outputSymbolList (const std::string &routineName, std::size_t level, TSymbolList &symbolList) {
    routineHeader.emplace_back (pcodeMemory.getOutputPosition (), createSymbolList (routineName, level, symbolList, std::vector<std::string> ()));
}

void TPCodeGenerator::createCodeListing () {
    std::size_t pos = 0;
    for (const std::pair<std::size_t, std::vector<std::string>> &routine: routineHeader) {
        pcodeMemory.appendListing (compilerListing, pos, routine.first);
        compilerListing.push_back (std::string ());
        compilerListing.insert (compilerListing.end (), routine.second.begin (), routine.second.end ());
        compilerListing.push_back (std::string ());
        pos = routine.first;
    }
    pcodeMemory.appendListing (compilerListing, pos, pcodeMemory.getOutputPosition ());
}

void TPCodeGenerator::assignStackOffsets (TSymbolList &symbolList) {
    ssize_t pos = 0;
    inherited::assignStackOffsets (pos, symbolList, true);
    
    const std::size_t alignReturnAddress = sizeof (std::size_t);
    pos = (pos + alignReturnAddress - 1) & ~(alignReturnAddress - 1);
    ssize_t parameterSize = pos;
    
    if (symbolList.getLevel ())  // no stack frame in startup code
        pos += 3 * sizeof (std::size_t);
    ssize_t variableStartPos = pos;
    
    inherited::assignStackOffsets (pos, symbolList, false);
    
    const std::size_t alignStack = TConfig::alignment - 1;
    pos = (pos + alignStack) & ~alignStack;
    
    for (TSymbol *s: symbolList)
        if (s->checkSymbolFlag (TSymbol::Alias))
            s->setAliasData ();
        else if (s->checkSymbolFlag (TSymbol::Parameter) || s->checkSymbolFlag (TSymbol::Variable))
            s->setOffset (s->getOffset () - variableStartPos);

    symbolList.setParameterSize (parameterSize);
    symbolList.setLocalSize (pos - variableStartPos);    
}

void TPCodeGenerator::initStaticRoutinePtr (std::size_t addr, const TRoutineValue *routineValue) {
    *reinterpret_cast<std::size_t *> (addr) = routineValue ->getSymbol ()->getOffset ();
}

void TPCodeGenerator::generateBlock (TBlock &block) {
    TSymbolList &blockSymbols = block.getSymbols ();
    assignStackOffsets (blockSymbols);
    
    beginRoutineBody (block.getSymbol ()->getName (), blockSymbols.getLevel (), blockSymbols);
    
    if (blockSymbols.getLevel () == 1) {
        assignGlobals (blockSymbols);
        pcodeMemory.setGlobalData (getGlobalDataArea ());
/*        
        TSymbol *s = blockSymbols.searchSymbol ("__globalruntimedata");
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadAddress, s->getLevel (), s->getOffset ()));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadRuntimePtr));
        pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::StorePtr));
*/        
    }
    
    visit (block.getStatements ());
    for (std::size_t pos: unresolvedExitCalls)
        pcodeMemory.fixupRoutineAddress (pos, pcodeMemory.getOutputPosition ());
    unresolvedExitCalls.clear ();
    endRoutineBody (blockSymbols.getLevel (), blockSymbols);
    
    for (TSymbol *s: blockSymbols) 
        if (s->checkSymbolFlag (TSymbol::Routine)) {
            s->setOffset (pcodeMemory.getOutputPosition ());
            for (std::size_t pos: s->getUnresolvedReferencePositions ())
                pcodeMemory.fixupRoutineAddress (pos, pcodeMemory.getOutputPosition ());
            if (s->checkSymbolFlag (TSymbol::External) | s->checkSymbolFlag (TSymbol::ExternalFFI)) 
                externalRoutine (*s);
            else {
                if (s->checkSymbolFlag (TSymbol::Export))
                    pcodeMemory.registerExportedRoutine (s->getName (), pcodeMemory.getOutputPosition (), getCallbackDescription (static_cast<TRoutineType *> (s->getType ())));
                visit (s->getBlock ());
            }
        }
    if (blockSymbols.getLevel () == 1) 
        initStaticGlobals (blockSymbols);
}

void TPCodeGenerator::generateRangeCheck (std::int64_t minval, std::int64_t maxval, bool vectorized, TPCode::TScalarTypeCode srcTypeCode) {
    if (codeRangeCheck) {
        if (vectorized) {
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, minval, 0));
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::LoadInt64Const, maxval, 0));
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::RangeCheckVec, static_cast<std::size_t> (srcTypeCode), 0));
        } else
            pcodeMemory.outputPCode (TPCodeOperation (TPCode::TOperation::RangeCheck, minval, maxval));
    }
}

} // namespace statpascal
