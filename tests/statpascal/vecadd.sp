program vecconv;

var
    v_s64: vector of int64;
    v_s32: vector of integer;
    v_s16: vector of smallint;
    v_s8:  vector of shortint;
    v_u32: vector of cardinal;
    v_u16: vector of word;
    v_u8:  vector of byte;
    v_r:   vector of real;

begin
    v_s8 := intvec (1, 10);
    v_u8 := v_s8;
    v_s16 := v_u8;
    v_u16 := v_s16;
    v_s32 := v_u16;
    v_u32 := v_s32;
    v_s64 := v_u32;
    v_r := v_s64;
    writeln (v_u8 + v_s8);
    writeln (v_u16 + v_s16);
    writeln (v_u32 + v_s32);
    writeln (v_s64 + v_r)
end.
