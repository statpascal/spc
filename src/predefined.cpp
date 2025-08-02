#include "predefined.hpp"
#include "codegenerator.hpp"

#include <iostream>
#include <map>
#include <unordered_map>

namespace statpascal {

class TRuntimeRoutine: public TExpressionBase {
using inherited = TExpressionBase;
public:
    TRuntimeRoutine (TType *returnType);
     
    virtual void acceptCodeGenerator (TCodeGenerator &) override;
   
    struct TFormatArguments {
        TExpressionBase *expression, *length, *precision;
    };
protected:
    void appendTransformedNode (TSyntaxTreeNode *);
    void checkFormatArguments (TFormatArguments &argument, const TType *outputtype, TBlock &);
    TExpressionBase *createAnyManagerIndex (const TType *type, TBlock &);
    
private:
    std::vector<TSyntaxTreeNode *> transformedNodes;
};

TRuntimeRoutine::TRuntimeRoutine (TType *returnType) {
    setType (returnType);
}

void TRuntimeRoutine::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    for (TSyntaxTreeNode *node: transformedNodes)
        codeGenerator.visit (node);
//    codeGenerator.generateCode (*this);
}

void TRuntimeRoutine::appendTransformedNode (TSyntaxTreeNode *node) {
    transformedNodes.push_back (node);
}

void TRuntimeRoutine::checkFormatArguments (TFormatArguments &argument, const TType *outputtype, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    if (argument.length)
        performTypeConversion (&stdType.Int64, argument.length, block);
    else
        argument.length = createInt64Constant (-1, block); 	
        
    if (argument.precision)
        if (outputtype == &stdType.Real)
            performTypeConversion (&stdType.Int64, argument.precision, block);
        else
            compiler.errorMessage (TCompilerImpl::InvalidType, "Decimal precision in format specifier only allowed for real types");
    else
        argument.precision = createInt64Constant (-1, block);
}            

TExpressionBase *TRuntimeRoutine::createAnyManagerIndex (const TType *type, TBlock &block) {
    return createInt64Constant (static_cast<TBaseGenerator &> (block.getCompiler ().getCodeGenerator ()).lookupAnyManager (type).runtimeIndex, block);
}


class TCombineRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TCombineRoutine (TBlock &, std::vector<TExpressionBase *> &&);
    
private:
    TFunctionCall *createCombineCall (TBlock &, TType *resultType, std::vector<TExpressionBase *> &&args);
};

TCombineRoutine::TCombineRoutine (TBlock &block, std::vector<TExpressionBase *> &&args):
  inherited (nullptr) {
    TType *resultType = args [0]->getType ();
    if (!resultType->isVector ())
        resultType = block.getCompiler ().createMemoryPoolObject<TVectorType> (resultType);
    setType (resultType);

    for (std::size_t i = 0; i < args.size (); ++i)
        if (!checkTypeConversion (resultType, args [i], block))
            block.getCompiler ().errorMessage (TCompilerImpl::InvalidType, "Cannot convert " + args [i]->getType ()->getName () + " to " + resultType->getName ());

    appendTransformedNode (createCombineCall (block, resultType, std::move (args)));
}

TFunctionCall *TCombineRoutine::createCombineCall (TBlock &block, TType *resultType, std::vector<TExpressionBase *> &&args) {
    static const std::array<std::string, 5> fn = {"", "", "__combine_vec_2", "__combine_vec_3", "__combine_vec_4"};
    if (args.size () <= 4)
        return createRuntimeCall (fn [args.size ()], resultType, std::move (args), block, false);
    std::size_t split = args.size () / 2;
    return createCombineCall (block, resultType,
         {createCombineCall (block, resultType, std::vector<TExpressionBase *> (args.begin (), args.begin () + split)),
          createCombineCall (block, resultType, std::vector<TExpressionBase *> (args.begin () + split, args.end ()))});
}


class TResizeRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TResizeRoutine (TBlock &, std::vector<TExpressionBase *> &&);
};

TResizeRoutine::TResizeRoutine (TBlock &block, std::vector<TExpressionBase *> &&args):
  inherited (&stdType.Void) {
    const TType *type = args [0]->getType ();
    appendTransformedNode (createRuntimeCall ("__resize_vec", &stdType.Void, {
        createAnyManagerIndex (type, block),
        createVariableAccess (TConfig::globalRuntimeDataPtr, block), 
        createInt64Constant (type->getSize (), block), args [0], args [1]},
        block, false));
}


class TNewRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TNewRoutine (TBlock &, std::vector<TExpressionBase *> &&);
};

TNewRoutine::TNewRoutine (TBlock &block, std::vector<TExpressionBase *> &&args):
  inherited (&stdType.Void) {
      const TType *type = args [0]->getType ()->getBaseType ();
      appendTransformedNode (createRuntimeCall ("__new", &stdType.Void, {
          static_cast<TLValueDereference *> (args [0])->getLValue (),
          args.size () == 2 ? args [1] : createInt64Constant (1, block),
          createInt64Constant (type->getSize (), block),
          createAnyManagerIndex (type, block),
          createVariableAccess (TConfig::globalRuntimeDataPtr, block)},
         block, false));
}


class TResetRewriteRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TResetRewriteRoutine (TBlock &, bool isReset, std::vector<TExpressionBase *> &&);
    
private:
    void checkArguments (TBlock &, bool isReset, std::vector<TExpressionBase *> &&);
};

TResetRewriteRoutine::TResetRewriteRoutine (TBlock &block, bool isReset, std::vector<TExpressionBase *> &&arguments):
  inherited (&stdType.Void) {
    checkArguments (block, isReset, std::move (arguments));
}

void TResetRewriteRoutine::checkArguments (TBlock &block, bool isReset, std::vector<TExpressionBase *> &&arguments) {
    TCompilerImpl &compiler = block.getCompiler ();
    const TFileType *type = static_cast<TFileType *> (arguments.front ()->getType ());
    const TType *baseType = type->getBaseType ();
    const bool isTextType = type == block.getSymbols ().searchSymbol ("text")->getType ();
    if (arguments.size () == 2) {
        if (isTextType || baseType)
            compiler.errorMessage (TCompilerImpl::InvalidType, "Record size can only be used with untyped files");
    } else if (!isTextType) 
        arguments.push_back (createInt64Constant (baseType ? baseType->getSize () : 128, block));
    arguments.push_back (TExpressionBase::createVariableAccess (TConfig::globalRuntimeDataPtr, block));
    const char *runtimeName [2][2] = {{"__rewrite_bin", "__rewrite_text"}, {"__reset_bin", "__reset_text"}};
    appendTransformedNode (createRuntimeCall (runtimeName [isReset][isTextType], nullptr, std::move (arguments), block, true));
}


class TStrRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TStrRoutine (TBlock &, std::vector<TRuntimeRoutine::TFormatArguments> &);
    
private:
    void checkArguments (TBlock &, std::vector<TRuntimeRoutine::TFormatArguments> &);
};

TStrRoutine::TStrRoutine (TBlock &block, std::vector<TRuntimeRoutine::TFormatArguments> &arguments):
  inherited (&stdType.Void) {
    checkArguments (block, arguments);
}

void TStrRoutine::checkArguments (TBlock &block, std::vector<TRuntimeRoutine::TFormatArguments> &arguments) {
    TRuntimeRoutine::TFormatArguments &valArgument = arguments.front ();
    const TType *type = convertBaseType (valArgument.expression, block);
    checkFormatArguments (valArgument, type, block);
    const std::string runtimeRoutine = (type == &stdType.Int64) ? "__str_int64" : "__str_dbl";

    std::vector<TExpressionBase *> callArguments {
        valArgument.expression, valArgument.length, valArgument.precision, arguments [1].expression};
    appendTransformedNode (createRuntimeCall (runtimeRoutine, nullptr, std::move (callArguments), block, true));
}

class TWriteRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TWriteRoutine (TBlock &, std::vector<TRuntimeRoutine::TFormatArguments> &, bool linefeed);
    
private:
    void checkArguments (TBlock &, std::vector<TRuntimeRoutine::TFormatArguments> &, bool linefeed);
};

TWriteRoutine::TWriteRoutine (TBlock &block, std::vector<TRuntimeRoutine::TFormatArguments> &arguments, bool linefeed):
  inherited (&stdType.Void) {
    checkArguments (block, arguments, linefeed);
}

void TWriteRoutine::checkArguments (TBlock &block, std::vector<TRuntimeRoutine::TFormatArguments> &arguments, bool linefeed) {
    TCompilerImpl &compiler = block.getCompiler ();
    static const std::map<std::pair<TType *, bool>, std::string> outputFunctionName = {
        {{&stdType.Int64, false}, "__write_int"},
        {{&stdType.Char, false}, "__write_char"},
        {{&stdType.Boolean, false}, "__write_boolean"},
        {{&stdType.Real, false}, "__write_dbl"},
        {{&stdType.String, false}, "__write_string"},
        {{&stdType.ShortString, false}, "__write_string"},
        
        {{&stdType.Int64, true}, "__write_vint64"},
        {{&stdType.Char, true}, "__write_vchar"},
        {{&stdType.Boolean, true}, "__write_vboolean"},
        {{&stdType.Real, true}, "__write_vdbl"},
        {{&stdType.String, true}, "__write_vstring"}
    };
    TExpressionBase *textfile = nullptr;
    if (arguments.empty () || !arguments.front ().expression->getType ()->isFile ())
        textfile = createVariableAccess ("output", block);

    for (TRuntimeRoutine::TFormatArguments &argument: arguments) 
        if (!textfile)
            textfile = argument.expression;
        else {
            TType *basetype = argument.expression->getType ();
            bool isVector = basetype->isVector ();
            if (isVector)
                basetype = getVectorBaseType (argument.expression, block);
            else if (basetype->isShortString ())
                basetype = &stdType.ShortString;	// generic version
            else
                basetype = convertBaseType (argument.expression, block);
            bool typeOk = basetype == &stdType.Real || basetype == &stdType.ShortString || basetype == &stdType.String || basetype == &stdType.Char || basetype == &stdType.Int64 || basetype == &stdType.Boolean;
            if (!typeOk) 
                compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot output value of type " + basetype->getName ());
            else {                
                checkFormatArguments (argument, basetype, block);
                std::vector<TExpressionBase *> args = {textfile, argument.expression, argument.length, argument.precision};
#ifndef CREATE_9900
                args.push_back (TExpressionBase::createVariableAccess (TConfig::globalRuntimeDataPtr, block));
#endif                
                appendTransformedNode (createRuntimeCall (outputFunctionName.at (std::make_pair (basetype, isVector)), nullptr, std::move (args), block, true));
            }
        }
        
    if (linefeed)
#ifdef CREATE_9900
        appendTransformedNode (createRuntimeCall ("__write_lf", nullptr, {textfile}, block, true));
#else    
        appendTransformedNode (createRuntimeCall ("__write_lf", nullptr, {textfile, createVariableAccess (TConfig::globalRuntimeDataPtr, block)}, block, true));
#endif        
}


class TReadRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TReadRoutine (TBlock &, std::vector<TExpressionBase *> &&, bool linefeed);
    
private:
    void checkArguments (TBlock &, std::vector<TExpressionBase *> &&, bool linefeed);
};

TReadRoutine::TReadRoutine (TBlock &block, std::vector<TExpressionBase *> &&args, bool linefeed):
  inherited (&stdType.Void) {
    checkArguments (block, std::move (args), linefeed);
}

void TReadRoutine::checkArguments (TBlock &block, std::vector<TExpressionBase *> &&args, bool linefeed) {
    TCompilerImpl &compiler = block.getCompiler ();
    static const std::map<TType *, std::string> inputFunctionName = {
        {&stdType.Int64, "__read_int64"},
        {&stdType.Char, "__read_char"},
        {&stdType.Real, "__read_dbl"},
        {&stdType.String, "__read_string"}
    };
    TExpressionBase *textfile = nullptr;
    if (args.empty () || !args.front ()->getType ()->isFile ())
        textfile = createVariableAccess ("input", block);
    
    for (TExpressionBase *arg: args)
        if (!textfile) 
            textfile = arg;
        else {
            if (!arg->isLValueDereference ()) {
                compiler.errorMessage (TCompilerImpl::VariableExpected, "L-value required as argument of 'read(ln)'");
                return;
            }
            TType *type = arg->getType (), *basetype = type;
            if (type->isSubrange ())
                basetype = type->getBaseType ();
            std::map<TType *, std::string>::const_iterator it = inputFunctionName.find (basetype);
            if (it != inputFunctionName.end ()) {
                TExpressionBase *inputFunctionCall = createRuntimeCall (it->second, nullptr, {textfile, TExpressionBase::createVariableAccess (TConfig::globalRuntimeDataPtr, block)}, block, true);
                appendTransformedNode (compiler.createMemoryPoolObject<TAssignment> (static_cast<TLValueDereference *> (arg)->getLValue (), inputFunctionCall));
            } else {
                compiler.errorMessage (TCompilerImpl::InvalidType, "Cannod read value of type '" + type->getName () + "'");
                return;
            }
       }
    if (linefeed)
        appendTransformedNode (createRuntimeCall ("__read_lf", nullptr, {textfile, createVariableAccess (TConfig::globalRuntimeDataPtr, block)}, block, true));
}


class TValRoutine: public TRuntimeRoutine {
using inherited = TRuntimeRoutine;
public:
    TValRoutine (TBlock &, std::vector<TExpressionBase *> &&);
    
private:
    void checkArguments (TBlock &, std::vector<TExpressionBase *> &&);
};

TValRoutine::TValRoutine (TBlock &block, std::vector<TExpressionBase *> &&args):
  inherited (&stdType.Void) {
    checkArguments (block, std::move (args));
}

void TValRoutine::checkArguments (TBlock &block, std::vector<TExpressionBase *> &&args) {
    TCompilerImpl &compiler = block.getCompiler ();
    const TType *type = args [1]->getType ();
    if (type->isSubrange ())
        type = type->getBaseType ();
    std::string rtName;    
    if (type->isReal () || type->isSingle ())
        rtName = "__val_dbl";
    else if (type == &stdType.Int64)
        rtName = "__val_int";
    else 
        compiler.errorMessage (TCompilerImpl::InvalidType, "Cannot apply 'val' to value of type '" + args [1]->getType ()->getName () + "'");
    if (!rtName.empty ())
        appendTransformedNode (compiler.createMemoryPoolObject<TAssignment> (
            static_cast<TLValueDereference *> (args [1])->getLValue (), 
            createRuntimeCall (rtName, nullptr, {args [0], args [2]}, block, true)));
}


namespace RoutineDescription {

using TParameter = std::uint32_t;

const TParameter
    Int_8 = 1,
    Int_16 = 1 << 18,	// TODO rearrange flsgs
    Void = 1 << 19,
    Int_64 = 1 << 1,
    Dbl = 1 << 2,
    String = 1 << 3,
    Char = 1 << 4,
    Bool = 1 << 5,
    ScalarTypes = Int_8 | Int_64 | Dbl | String | Char | Bool,
    // LValue can be combined with vector or pointer
    Generic = 1 << 6,
    LValueRequired = 1 << 7,	
    Pointer = 1 << 8,
    File = 1 << 9,
    Vector = 1 << 10,
    FormatSpec = 1 << 11,
    TextFile = 1 << 13,
    UntypedFile = 1 << 14,
    TypedFile = 1 << 15,
    Enumerated = 1 << 16,
    DerefLValueRequired = 1 << 17;	// usual convention for routine calls
    
enum TName {
    Chr, 
    Str, Val,
    New, Dispose,
    Reset, Rewrite,                   
    RuntimeCall,
    Addr, Ord, Odd, Succ, Pred, Inc, Dec, Write, Writeln, Read, Readln, Combine, Resize,
    Exit, Break, Halt
};

enum {SpecialParser = 1, AppendGlobalRuntimeDataPtr = 2, RuntimeNoParaCheck = 4, KeepType = 8};
    
struct TRoutine {
    const TName name;
    const TParameter returnType;
    const std::vector<TParameter> parameterDescription;
    const std::string runtimeName;
    const std::uint8_t flags;
    
//    TRoutine (): name (RuntimeCall), returnType (Int_8), flags (0) {}
};
    
using TRoutineMap = std::unordered_map<std::string, std::vector<TRoutine>>;
    
const TRoutineMap routineMap = {
    {"chr", 	   {{Chr, Char, {Int_8}}}},
    {"ord", 	   {{Ord, Int_64, {Enumerated}}}},
    
    {"new", 	   {{New, Void, {Pointer | LValueRequired}},
                    {New, Void, {Pointer | LValueRequired, Int_64}}}},
    {"dispose",    {{RuntimeCall, Void, {Pointer}, "__dispose", AppendGlobalRuntimeDataPtr}}},
    
    {"resize", 	   {{Resize, Void, {Vector | LValueRequired, Int_64}, ""}}},
    {"size",	   {{RuntimeCall, Int_64, {Vector}, "__size_vec", RuntimeNoParaCheck}}},
    {"rev", 	   {{RuntimeCall, Vector, {Vector}, "__rev_vec", RuntimeNoParaCheck | KeepType}}},
    
    {"assign", 	   {{RuntimeCall, Void, {File | DerefLValueRequired, String}, "__assign"}}},
    {"reset", 	   {{Reset, Void, {File | DerefLValueRequired}},
                    {Reset, Void, {File | DerefLValueRequired, Int_64}}}},
    {"rewrite",    {{Rewrite, Void, {File | DerefLValueRequired}},
                    {Rewrite, Void, {File | DerefLValueRequired, Int_64}}}},
    {"blockwrite", {{RuntimeCall, Void, {UntypedFile | DerefLValueRequired, Generic | DerefLValueRequired, Int_64}, "__write_bin_ign"},
                    {RuntimeCall, Void, {UntypedFile | DerefLValueRequired, Generic | DerefLValueRequired, Int_64, Int_64 | DerefLValueRequired}, "__write_bin", AppendGlobalRuntimeDataPtr}}},
    {"blockread",  {{RuntimeCall, Void, {UntypedFile | DerefLValueRequired, Generic | DerefLValueRequired, Int_64}, "__read_bin_ign"},
                    {RuntimeCall, Void, {UntypedFile | DerefLValueRequired, Generic | DerefLValueRequired, Int_64, Int_64 | DerefLValueRequired}, "__read_bin", AppendGlobalRuntimeDataPtr}}},
    {"seek",       {{RuntimeCall, Void, {UntypedFile | DerefLValueRequired, Int_64}, "__seek_bin", AppendGlobalRuntimeDataPtr},
                    {RuntimeCall, Void, {TypedFile | DerefLValueRequired, Int_64}, "__seek_bin", AppendGlobalRuntimeDataPtr}}},
    {"filesize",   {{RuntimeCall, Int_64, {UntypedFile | DerefLValueRequired}, "__filesize_bin", AppendGlobalRuntimeDataPtr},
                    {RuntimeCall, Int_64, {TypedFile | DerefLValueRequired}, "__filesize_bin", AppendGlobalRuntimeDataPtr}}},
    {"filepos",    {{RuntimeCall, Int_64, {UntypedFile | DerefLValueRequired}, "__filepos_bin", AppendGlobalRuntimeDataPtr},
                    {RuntimeCall, Int_64, {TypedFile | DerefLValueRequired}, "__filepos_bin", AppendGlobalRuntimeDataPtr}}},
    {"flush", 	   {{RuntimeCall, Void, {File | DerefLValueRequired}, "__flush", AppendGlobalRuntimeDataPtr}}},
    {"erase", 	   {{RuntimeCall, Void, {File | DerefLValueRequired}, "__erase"}}},
    {"close", 	   {{RuntimeCall, Void, {File | DerefLValueRequired}, "__close", AppendGlobalRuntimeDataPtr}}},
    {"eof", 	   {{RuntimeCall, Bool, {}, "__eof_input"},
                    {RuntimeCall, Bool, {File | DerefLValueRequired}, "__eof", AppendGlobalRuntimeDataPtr}}},
    
    {"val", 	   {{Val, Void, {String, DerefLValueRequired, Int_16}}}},
    {"addr", 	   {{Addr, Pointer, {LValueRequired}}}},
    {"odd", 	   {{Odd, Bool, {Int_64}}}}, 
    {"succ", 	   {{Succ, Void, {Enumerated}, "", KeepType}}},
    {"pred", 	   {{Pred, Void, {Enumerated}, "", KeepType}}},
    {"inc", 	   {{Inc, Void, {Enumerated | LValueRequired}},
                    {Inc, Void, {Enumerated | LValueRequired, Int_64}},
                    {Inc, Void, {Pointer | LValueRequired}},
                    {Inc, Void, {Pointer | LValueRequired, Int_64}}}},
    {"dec", 	   {{Dec, Void, {Enumerated | LValueRequired}},
                    {Dec, Void, {Enumerated | LValueRequired, Int_64}},
                    {Dec, Void, {Pointer | LValueRequired}},
                    {Dec, Void, {Pointer | LValueRequired, Int_64}}}},
             
    {"str", 	   {{Str, Void, {Int_64 | FormatSpec, String | DerefLValueRequired}},
                    {Str, Void, {Dbl | FormatSpec, String | DerefLValueRequired}}}},
    {"write", 	   {{Write, Void, {TypedFile | DerefLValueRequired, Generic | DerefLValueRequired}, "__write_bin_typed"}}},
    {"writeln",    {{Writeln, Void, {}, "", SpecialParser}}},
    {"read", 	   {{Read, Void, {TypedFile | DerefLValueRequired, Generic | DerefLValueRequired}, "__read_bin_typed"}}},
    {"readln", 	   {{Readln, Void, {}, "", SpecialParser}}},
    {"combine",    {{Combine, Void, {}, "", SpecialParser}}},
    {"exit", 	   {{Exit, Void, {}}}}
};

const std::map<RoutineDescription::TName, TPredefinedRoutine::TRoutine> predefinedMap = {
    {RoutineDescription::Odd, 		TPredefinedRoutine::Odd},
    {RoutineDescription::Succ, 		TPredefinedRoutine::Succ},
    {RoutineDescription::Pred, 		TPredefinedRoutine::Pred},
    {RoutineDescription::Inc, 		TPredefinedRoutine::Inc},
    {RoutineDescription::Dec,		TPredefinedRoutine::Dec},
    {RoutineDescription::Exit, 		TPredefinedRoutine::Exit}
};

const std::map<TParameter, TType *> typeMap = {
    {Int_8, &stdType.Uint8},
    {Int_16, &stdType.Uint16},
    {Int_64, &stdType.Int64},
    {Dbl, &stdType.Real},
    {String, &stdType.String},
    {Char, &stdType.Char},
    {Bool, &stdType.Boolean},
    {Pointer, &stdType.GenericPointer},
    {Void, &stdType.Void}
};

TType *getType (RoutineDescription::TParameter p, TCompilerImpl &compiler) {
    const std::map<TParameter, TType *>::const_iterator it = typeMap.find (p);
    return  it == typeMap.end () ? nullptr : it->second;
}
} // namespace RoutineScription

TPredefinedRoutine::TPredefinedRoutine (TRoutine routine, TType *returnType, std::vector<TExpressionBase *> &&args):
  routine (routine),
  arguments (std::move (args)) {
    setType (returnType);
}

TPredefinedRoutine::TRoutine TPredefinedRoutine::getRoutine () const {
    return routine;
}

const std::vector<TExpressionBase *> &TPredefinedRoutine::getArguments () const {
    return arguments;
}

void TPredefinedRoutine::acceptCodeGenerator (TCodeGenerator &codeGenerator) {
    codeGenerator.generateCode (*this);
}

TExpressionBase *TPredefinedRoutine::parse (const std::string &identifier, TBlock &block) {
    TCompilerImpl &compiler = block.getCompiler ();
    TLexer &lexer = block.getLexer ();
    
    RoutineDescription::TRoutineMap::const_iterator it = RoutineDescription::routineMap.find (identifier);
    if (it == RoutineDescription::routineMap.end ()) {
        compiler.errorMessage (TCompilerImpl::IdentifierExpected, "'" + identifier + "' is undefined");
        return nullptr;
    }
    const std::vector<RoutineDescription::TRoutine> &routineDescriptions = it->second;

    bool success = true;
    std::vector<TRuntimeRoutine::TFormatArguments> formatArguments;
    std::vector<TExpressionBase *> args;
    if (lexer.checkToken (TToken::BracketOpen)) {
        do {
            TExpressionBase *expression = TExpression::parse (block);
            if (expression && expression->getType ()) {
                args.push_back (expression);
                formatArguments.push_back ({expression, nullptr, nullptr});
                if (lexer.checkToken (TToken::Colon))
                    formatArguments.back ().length = TExpression::parse (block);
                if (lexer.checkToken (TToken::Colon))
                    formatArguments.back ().precision = TExpression::parse (block);
            } else
                success = false;
        } while (lexer.checkToken (TToken::Comma));
        if (!lexer.checkToken (TToken::BracketClose))
            compiler.errorMessage (TCompilerImpl::SyntaxError, "Missing ')' in call to subroutine '" + identifier + "'");
    }
    if (!success)
        return nullptr;
    
    struct TErrorMessage {
        TCompilerImpl::TErrorType errorType;
        std::string errorMsg;
    } errorMessage;

    for (const RoutineDescription::TRoutine &routineDescription: routineDescriptions) {
        if (formatArguments.size () == routineDescription.parameterDescription.size () && !(routineDescription.flags & RoutineDescription::SpecialParser)) {
            bool success = true;
            for (std::size_t i = 0; success && i < routineDescription.parameterDescription.size (); ++i) {
                const RoutineDescription::TParameter parameterDescription = routineDescription.parameterDescription [i];
                const TType *type = args [i]->getType ();
                
                if (parameterDescription & (RoutineDescription::LValueRequired | RoutineDescription::DerefLValueRequired)) {
                    if (!args [i]->isLValueDereference ()) {
                        errorMessage = {TCompilerImpl::VariableExpected, "L-value required as argument of " + identifier};
                        success = false;
                    }
                }
                if (!(parameterDescription & RoutineDescription::FormatSpec) && formatArguments [i].length) {
                    success = false;
                    errorMessage = {TCompilerImpl::InvalidUseOfSymbol, "Format specifiers are not allowed as argument of " + identifier};
                }
                if (parameterDescription & RoutineDescription::TextFile) {
                    if (type != block.getSymbols ().searchSymbol ("text")->getType ()) {
                        errorMessage = {TCompilerImpl::InvalidType, "Text file type required as argument of " + identifier};
                        success = false;
                    }
                } else if (parameterDescription & RoutineDescription::File) {
                    if (!type->isFile ()) {
                        errorMessage = {TCompilerImpl::InvalidType, "File type required as argument of " + identifier};
                        success = false;
                    }
                } else if (parameterDescription & RoutineDescription::TypedFile) {
                    if (!type->isFile () || !type->getBaseType ())
                        success = false;
                } else if (parameterDescription & RoutineDescription::UntypedFile) {
                    if (!type->isFile () || type->getBaseType ())
                        success = false;
                } else if (parameterDescription & RoutineDescription::Enumerated) {
                    if (!type->isEnumerated ()) {
                        errorMessage = {TCompilerImpl::InvalidType, "Enumerated type required as argument of " + identifier};
                        success = false;
                    }
                } else if (parameterDescription & RoutineDescription::Pointer) {
                    if (!type->isPointer ()) {
                        errorMessage  = {TCompilerImpl::InvalidType, "Pointer type required as argument of " + identifier};
                        success = false;
                    }
                } else if (parameterDescription & RoutineDescription::Vector) {
                    if (!type->isVector ()) {
                        errorMessage = {TCompilerImpl::InvalidType, "Vector type required as argument of " + identifier};
                        success = false;
                    }
                    if (parameterDescription & RoutineDescription::ScalarTypes)
                        success = checkTypeConversionVector (compiler.createMemoryPoolObject<TVectorType> (RoutineDescription::getType (parameterDescription & RoutineDescription::ScalarTypes, compiler)), args [i], block);
                } else
                    success = checkTypeConversion (RoutineDescription::getType (parameterDescription & RoutineDescription::ScalarTypes, compiler), args [i], block);
            }
            if (success) {
                TType *returnType = RoutineDescription::getType (routineDescription.returnType, compiler);
                bool createCast = false;
                switch (routineDescription.name) {
                    case RoutineDescription::Str:
                        return compiler.createMemoryPoolObject<TStrRoutine> (block, formatArguments);
                    case RoutineDescription::Val:
                        return compiler.createMemoryPoolObject<TValRoutine> (block, std::move (args));
                    case RoutineDescription::Reset:
                    case RoutineDescription::Rewrite:
                        return compiler.createMemoryPoolObject<TResetRewriteRoutine> (block, routineDescription.name == RoutineDescription::Reset, std::move (args));
                    case RoutineDescription::Addr:
                        if (args [0]->isSymbol ())
                            static_cast<TVariable *> (args [0])->getSymbol ()->setAliased ();
                    case RoutineDescription::Ord:
                    case RoutineDescription::Chr:
                        createCast = true;
                        break;
                    case RoutineDescription::Resize:
                        return compiler.createMemoryPoolObject<TResizeRoutine> (block, std::move (args));
                    case RoutineDescription::New:
                        return compiler.createMemoryPoolObject<TNewRoutine> (block, std::move (args));
                    default:
                        break;
                }
                
                if (routineDescription.flags & RoutineDescription::KeepType)
                    returnType = args [0]->getType ();
                
                if (routineDescription.flags & RoutineDescription::AppendGlobalRuntimeDataPtr)
                    args.push_back ({TExpressionBase::createVariableAccess (TConfig::globalRuntimeDataPtr, block)});
                    
                if ((routineDescription.name == RoutineDescription::Pred || routineDescription.name == RoutineDescription::Succ) && returnType->isEnumerated () && args [0]->isConstant ()) 
                    return createConstant<std::int64_t> (static_cast<TConstantValue *> (args [0])->getConstant ()->getInteger () + (routineDescription.name == RoutineDescription::Succ ? 1 : - 1), returnType, block);
                else if (!routineDescription.runtimeName.empty ())
                    return createRuntimeCall (routineDescription.runtimeName, returnType, std::move (args), block, !(routineDescription.flags & RoutineDescription::RuntimeNoParaCheck));
                else {
                    for (std::size_t i = 0; i < args.size (); ++i)
                        if (args [i]->isLValueDereference () && (routineDescription.parameterDescription [i] & RoutineDescription::LValueRequired))
                            args [i] = static_cast<TLValueDereference *> (args [i])->getLValue ();
                    if (createCast)
                        return compiler.createMemoryPoolObject<TTypeCast> (returnType, args [0]);
                    else
                        return compiler.createMemoryPoolObject<TPredefinedRoutine> (
                            static_cast<TPredefinedRoutine::TRoutine> (RoutineDescription::predefinedMap.at (routineDescription.name)), returnType, std::move (args));
                }
            }
        }
    }

    switch (routineDescriptions.front ().name) {
        case RoutineDescription::Write:
             return compiler.createMemoryPoolObject<TWriteRoutine> (block, formatArguments, false);
        case RoutineDescription::Writeln:
             return compiler.createMemoryPoolObject<TWriteRoutine> (block, formatArguments, true);
        case RoutineDescription::Read:
            return compiler.createMemoryPoolObject<TReadRoutine> (block, std::move (args), false);
        case RoutineDescription::Readln: 
            return compiler.createMemoryPoolObject<TReadRoutine> (block, std::move (args), true);
        case RoutineDescription::Combine:
            if (!args.empty ())
                return compiler.createMemoryPoolObject<TCombineRoutine> (block, std::move (args));
        case RoutineDescription::Resize:
            return compiler.createMemoryPoolObject<TResizeRoutine> (block, std::move (args));
        default:
            break;
    }
    
    if (!errorMessage.errorMsg.empty ())
        compiler.errorMessage (errorMessage.errorType, errorMessage.errorMsg);
    else
        compiler.errorMessage (TCompilerImpl::IdentifierExpected, "'" + identifier + "' is called with wrong arguments");
    return nullptr;
}

}
