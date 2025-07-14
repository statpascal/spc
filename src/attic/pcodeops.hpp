#pragma once

#include <string>

namespace statpascal {

struct TPCode {
    enum class TOperation {
        LoadInt64Const, LoadDoubleConst, LoadExportAddr,
        LoadAddress, CalcIndex, StrIndex, StrPtr,
        LoadAcc, StoreAcc, LoadRuntimePtr,
        
        LoadInt8, LoadInt16, LoadInt32, LoadInt64, LoadUint8, LoadUint16, LoadUint32, LoadDouble, LoadSingle, LoadVec, LoadPtr,
        Store8, Store16, Store32, Store64, StoreDouble, StoreSingle, StoreVec, StorePtr,
        Push8, Push16, Push32, Push64, PushDouble, PushSingle, PushVec, PushPtr,	// push from calculator stack
        
        CopyMem, CopyMemAny, CopyMemVec, PushMem, PushMemAny, PushMemVec, AlignStack, Destructors, ReleaseStack,
        Alloc, Free,
        
        CallStack, Call, CallFFI, CallExtern, Enter, Leave, Return, Jump, JumpZero, JumpNotZero,
        JumpAccEqual, CmpAccRange,
        
        AddInt64Const, Dup,
        
        AddInt, SubInt, OrInt, XorInt, MulInt, DivInt, ModInt, AndInt, ShlInt, ShrInt,
        EqualInt, NotEqualInt, LessInt, GreaterInt, LessEqualInt, GreaterEqualInt,
        
        NotInt, NegInt, NegDouble, NotBool,
        
        AddDouble, SubDouble, MulDouble, DivDouble,
        EqualDouble, NotEqualDouble, LessDouble, GreaterDouble, LessEqualDouble, GreaterEqualDouble,
        
        ConvertIntDouble,
        
        MakeVec, MakeVecMem, MakeVecZero, ConvertVec, CombineVec, ResizeVec,
        VecIndexIntVec, VecIndexBoolVec, VecIndexInt,
        
        NotIntVec, NotBoolVec,
        AddVec, SubVec, OrVec, XorVec, MulVec, DivVec, DivIntVec, ModVec, AndVec,
        EqualVec, NotEqualVec, LessVec, GreaterVec, LessEqualVec, GreaterEqualVec,
        
        VecLength, VecRev, RangeCheckVec,
        
        RangeCheck, NoOperation, Stop, Break, Halt, IllegalOperation
    };
    
    enum class TScalarTypeCode {
        s64, s32, s16, s8, u8, u16, u32, single, real, count, invalid
    };
    
    static const std::size_t scalarTypeSizes [static_cast<std::size_t> (TPCode::TScalarTypeCode::count)];
    static const std::string scalarTypeNames [static_cast<std::size_t> (TPCode::TScalarTypeCode::count)];
};

}
