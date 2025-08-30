unit system;

interface

type
    integer = int16;
    PChar = ^char;
    string1 = string [1];
    string2 = string [2];
    string4 = string [4];
    
    TVdpRegList = array [0..7] of uint8;
    
var
    input, output: text;
    memb: array [0..65535] of uint8 absolute $0000;
    memw: array [0..32767] of integer absolute $0000;
    
procedure gotoxy (x, y: integer);
procedure clrscr;

procedure move (var src, dest; length: integer);
function compareWord (var src, dest; length: integer): boolean;

procedure vmbw (var src; dest, length: integer);	// video multiple byte write
procedure vmbr (var dest; src, length: integer);	// video multiple byte read
procedure vrbw (val: uint8; length: integer);		// video repeated byte write
procedure writeV (addr: integer; val: uint8);
function readV (addr: integer): uint8;

function min (a, b: integer): integer;
function max (a, b: integer): integer;
function sqr (a: integer): integer;
function abs (n: integer): integer; external;

procedure __write_lf (var f: text);
procedure __write_int (var f: text; n, length, precision: integer);
procedure __write_char (var f: text; ch: char; length, precision: integer);
procedure __write_string (var f: text; p: PChar; length, precision: integer);
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);

function length (s: PChar): integer;
function pos (ch: char; s: PChar): integer;
function copy (s: PChar; start, len: integer): string;

function __short_str_char (ch: char): string1;
function __short_str_concat (a, b: PChar): string;
function __short_str_equal (s, t: PChar): boolean;
function __short_str_not_equal (s, t: PChar): boolean; 
function __short_str_less_equal (s, t: PChar): boolean; 
function __short_str_greater_equal (s, t: PChar): boolean; 
function __short_str_less (s, t: PChar): boolean; 
function __short_str_greater (s, t: PChar): boolean; 

function keypressed: boolean;
procedure waitkey;

procedure __new (var p: pointer; count, size: integer);
procedure __dispose (p: pointer);
procedure mark (var p: pointer);
procedure release (p: pointer);

procedure setCRUBit (addr: integer; val: boolean);

const
    WriteAddr = $4000;
    
var
    vdprd:  char absolute $8800;
    vdpsta: char absolute $8802;
    vdpwd:  char absolute $8c00;
    vdpwa:  char absolute $8c02;
    gromwa: char absolute $9c02;
    gromrd: char absolute $9800;
    
procedure setVdpRegs (var vdpRegs: TVdpRegList);
procedure setVdpAddress (n: integer);

function hexstr (n: integer): string4;
function hexstr2 (n: uint8): string2;

const
    __set_words = 16;

type
    __generic_set_type = set of 0..255;
    __set_array = array [0..__set_words - 1] of integer;
    
function __set_add_val (val: integer; var s: __set_array): __generic_set_type; 
function __set_add_range (minval, maxval: integer; var s: __set_array): __generic_set_type; 

function __in_set (v: integer; var s: __set_array): boolean;
function __set_union (var s, t: __set_array): __generic_set_type; 
function __set_intersection (var s, t: __set_array): __generic_set_type;
function __set_diff (var s, t: __set_array): __generic_set_type;

function __set_equal (var s, t: __set_array): boolean;
function __set_not_equal (var s, t: __set_array): boolean;
function __set_sub (var s, t: __set_array): boolean;
function __set_super (var s, t: __set_array): boolean;
function __set_sub_not_equal (var s, t: __set_array): boolean;
function __set_super_not_equal (var s, t: __set_array): boolean;

    
implementation

var 
    vdpWriteAddress: integer;
    
// simple UCSD-like heap managment in lower memory

const
    heapPtr: integer = $4000;
    heapMin = $2000;
    
procedure __new (var p: pointer; count, size: integer);
    var
        n: integer;
    begin
        n := (count * size + 1) and not 1;
        if heapPtr - n < heapMin then
            p := nil
        else 
            begin
                dec (heapPtr, n);
                p := pointer (heapPtr)
            end
    end;
    
procedure __dispose (p: pointer);
    begin
        // nothing yet
    end;
    
procedure mark (var p: pointer);
    begin
        p := pointer (heapPtr)
    end;
    
procedure release (p: pointer);
    begin
        heapPtr := integer (p)
    end;

// 

procedure setCRUBit (addr: integer; val: boolean); assembler;
    mov  *r10, r12
    mov  @2(r10), r13
    ldcr r13, 1
end;

procedure setVdpAddress (n: integer);
    begin
        vdpwa := chr (n and 255);
        vdpwa := chr (n shr 8)
    end;
    
procedure _rt_scroll_up; assembler;
        li r0, 32
        li r13, vdpwa
        li r14, vdprd
        li r15, vdpwd
        
    _rt_scroll_up_1:
        swpb r0
        movb r0, *r13
        swpb r0
        movb r0, *r13
        li r8, 8
        li r12, >8322
        
    _rt_scroll_up_2:
        movb *r14, *r12+
        movb *r14, *r12+
        movb *r14, *r12+
        movb *r14, *r12+
        dec r8
        jne _rt_scroll_up_2

        ai r0, >3fe0
        swpb r0
        movb r0, *r13
        swpb r0
        movb r0, *r13

        li r8, 8
        li r12, >8322
        
    _rt_scroll_up_3:
        movb *r12+, *r15
        movb *r12+, *r15
        movb *r12+, *r15
        movb *r12+, *r15
        dec r8
        jne _rt_scroll_up_3

        ai r0, >c040
        ci r0, 768
        jne _rt_scroll_up_1

        li r8, 32
        li r12, >2000
        
    _rt_scroll_up_4:
        movb r12, *r15
        dec r8
        jne _rt_scroll_up_4
end;

procedure move (var src, dest; length: integer); assembler;
        mov @src, r12
        mov @dest, r13
        mov @length, r14
        jeq move_2
        
    move_1:    
        movb *r12+, *r13+
        dec r14
        jne move_1
        
    move_2:
end;

function compareWord (var src, dest; length: integer): boolean; assembler;
        mov *r10, r15
        mov @src, r12
        mov @dest, r13
        mov @length, r14
        jeq comparebyte_2
        
    comparebyte_1:
        c *r12+, *r13+
        jne comparebyte_3
        dec r14
        jne comparebyte_1
        
    comparebyte_2:
        li r12, >0100
        movb r12, *r15
        b *r11
        
    comparebyte_3
        clr r12
        movb r12, *r15
        b *r11
end;
    
procedure vrbw (val: uint8; length: integer); assembler;
        mov @length, r12
        li r13, vdpwd
        movb @val, r14
    vrbw_1:
        movb r14, *r13
        dec r12
        jne vrbw_1
end;

procedure writeV (addr: integer; val: uint8);
    begin
        vdpwa := chr (addr and 255);
        vdpwa := chr (addr shr 8 or $40);
        vdpwd := chr (val)
    end;
    
function readV (addr: integer): uint8;
    begin
        vdpwa := chr (addr and 255);
        vdpwa := chr (addr shr 8);
        readV := ord (vdprd)
    end;

procedure __write_data (p: pointer; length: integer); assembler;
        mov @length, r12
        jeq __write_data_2
        mov @p, r13
        li r14, vdpwd
        inc r13
    __write_data_1:
        movb *r13+, *r14
        dec r12
        jne __write_data_1
    __write_data_2
end;

procedure vmbw (var src; dest, length: integer); assembler;
        mov @length, r14
        jeq vmbw_2
        
        mov @src, r12
        mov @dest, r13
        li r15, vdpwd

        ori r13, >4000
        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa

    vmbw_1:
        movb *r12+, *r15
        dec r14
        jne vmbw_1
        
    vmbw_2:
end;
    
procedure vmbr (var dest; src, length: integer); assembler;
        mov @length, r14
        jeq vmbr_2
        
        mov @dest, r12
        mov @src, r13
        li r15, vdprd

        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa

    vmbr_1:
        movb *r15, *r12+
        dec r14
        jne vmbr_1
    vmbr_2:
end;

procedure gotoxy (x, y: integer);
    begin
        vdpWriteAddress := y shl 5 + x;
        setVdpAddress (vdpWriteAddress or WriteAddr)
    end;
    
procedure clrscr;
    begin
        setVdpAddress (WriteAddr);
        vrbw (32, 768);
        gotoxy (0, 0)
    end;
    
procedure scroll;
    begin
       _rt_scroll_up;
        dec (vdpWriteAddress, 32);
    end;
    
procedure __write_lf (var f: text);
    begin
        vdpWriteAddress := (vdpWriteAddress + 32) and not 31;
        if vdpWriteAddress = 24 * 32 then
            scroll;
        setVdpAddress (vdpWriteAddress or WriteAddr);
    end;
    
procedure __write_int (var f: text; n, length, precision: integer);

    function outint (n: integer; neg: boolean; p: pointer): pointer; assembler;
            mov @p, r0
            mov @n, r14
            li r12, 10
        
        outint1:
            clr r13
            div r12, r13
            ai r14, 48
            swpb r14
            movb r14, *r0
            dec r0
            mov r13, r14
            jne outint1
            
            mov @neg, r13
            jeq outint2
            li r13, >2d00
            movb r13, *r0
            dec r0
            
        outint2:
            mov *r10, r12
            mov r0, *r12
    end;
        
    var
        buf: string [6];
        p: PChar;
    begin
        if n = -32768 then
            __write_string (f, '-32768', length, precision)
        else
            begin
                p := outint (abs (n), n < 0, addr (buf [6]));
                p^ := chr (integer (addr (buf [6])) - integer (p));
                __write_string (f, p, length, -1)
            end
    end;
    
procedure __write_char (var f: text; ch: char; length, precision: integer);
    var
        s: string [1];
    begin
        s [0] := #1;
        s [1] := ch;
        __write_string (f, s, length, -1);
    end;
    
procedure __write_string (var f: text; p: PChar; length, precision: integer);
    var
        len, outlen: integer;
    begin
        len := ord (p^);
        outlen := len;
        if length > len then
            outlen := length;
        while vdpWriteAddress + outlen > 24 * 32 do
            scroll;
        setVdpAddress (vdpWriteAddress or WriteAddr);
        inc (vdpWriteAddress, outlen);
        if outlen > len then
            vrbw (32, outlen - len);
        __write_data (p, len)
    end;
    
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);
    begin
        if b then
            __write_string (f, 'true', length, -1)
        else
            __write_string (f, 'false', length, -1) 
    end;

function min (a, b: integer): integer; assembler;
        mov *r10, r12   // pointer to result
        c @a, @b
        jlt min_1
        mov @b, *r12
        jmp min_2
    min_1:
        mov @a, *r12
    min_2:
end;
(*
    begin
        if a < b then
            min := a
        else
            min := b
    end;
*)    

function max (a, b: integer): integer;
    begin
        if a < b then
            max := b
        else
            max := a
    end;

function sqr (a: integer): integer;
    begin
        sqr := a * a
    end;
    
function __short_str_char (ch: char): string1;
    begin
        result [0] := #1;
        result [1] := ch
    end;

// TODO: pass max. length of result string - this will crash if too short

function __short_str_concat (a, b: PChar): string;
    begin
        move (a^, result, ord (a^) + 1);
        move (b [1], result [ord (a^) + 1], min (ord (b^), 255 - ord (a^)));
        result [0] := chr (min (255, ord (a^) + ord (b^)))
    end;

function strCompare (s, t: PChar): integer;
    var 
        i: integer;
    begin
        for i := 1 to min (ord (s^), ord (t^)) do
            if s [i] <> t [i] then
                begin
                    strCompare := ord (s [i]) - ord (t [i]);
                    exit
                end;
        strCompare := ord (s^) - ord (t^)
    end;
    
function __short_str_equal (s, t: PChar): boolean;
    begin
        __short_str_equal := strCompare (s, t) = 0
    end;
    
function __short_str_not_equal (s, t: PChar): boolean; 
    begin
        __short_str_not_equal := strCompare (s, t) <> 0
    end;
    
function __short_str_less_equal (s, t: PChar): boolean; 
    begin
        __short_str_less_equal := strCompare (s, t) <= 0
    end;
    
function __short_str_greater_equal (s, t: PChar): boolean; 
    begin
        __short_str_greater_equal := strCompare (s, t) >= 0
    end;
    
function __short_str_less (s, t: PChar): boolean; 
    begin
        __short_str_less := strCompare (s, t) < 0
    end;
    
function __short_str_greater (s, t: PChar): boolean; 
    begin
        __short_str_greater := strCompare (s, t) > 0
    end;
    
function length (s: PChar): integer;
    begin
        length := ord (s^)
    end;
    
function pos (ch: char; s: PChar): integer;
    var
        i: integer;
    begin
        for i := 1 to ord (s^) do
            if s [i] = ch then
                begin
                    pos := i;
                    exit
                end;
        pos := 0
    end;
    
// TODO: pass max. length of result string - this will crash if too short

function copy (s: PChar; start, len: integer): string;
    begin
        if start <= ord (s^) then
            begin
                if start + len > succ (ord (s^)) then
                    len := succ (ord (s^) - start);
                move (s [start], result [1], len);
                result [0] := chr (len)
            end
        else
            result [0] := #0
    end;


const 
    hex: string [16] = '0123456789ABCDEF';
    
function hexstr (n: integer): string4;
    var
        i: integer;
    begin
        for i := 1 to 4 do
            result [i] := hex [succ (n shr (16 - i shl 2) and $0f)];
        result [0] := #4
    end;
    
function hexstr2 (n: uint8): string2;
    begin
        result [0] := #2;
        result [1] := hex [succ (n shr 4)];
        result [2] := hex [succ (n and $0f)]
    end;
    
function keypressed: boolean; assembler;
        clr r14
        clr r13
        li r15, >0100   // true
        
    keypressed_1:
        li r12, >0024
        ldcr r13, 3
        li r12, >0006
        stcr r14, 8
        ci r14, >ff00
        jne keypressed_2
        ai r13, >0100
        ci r13, >0600
        jne keypressed_1
        clr r15         // return false
    
    keypressed_2:
        mov *r10, r12
        movb r15, *r12
    end;

procedure waitkey;
    begin
        repeat
        until keypressed
    end;
    
procedure loadCharset;
    var 
        i, j: integer;
    begin
        gromwa := #$06; // >06b4: standard char set
        gromwa := #$b4;
        setVdpAddress ($0800 + 8 * 32 or WriteAddr);
        for i := 32 to 127 do
            begin
                for j := 1 to 7 do
                    vdpwd := gromrd;
                vdpwd := chr (0)
            end
    end;

procedure setVdpRegs (var vdpRegs: TVdpRegList);
    var 
        i: integer;
        dummy: char;
    begin
        dummy := vdpsta;
        for i := 0 to 7 do
            begin
                vdpwa := chr (vdpRegs [i]);
                vdpwa := chr ($80 + i)
            end
    end;
    
    
type
    PSet = ^__set_array;
    PPSet = ^PSet;

function __set_add_val (val: integer; var s: __set_array): __generic_set_type; 
    begin
        __set_array (result) := s;
        __set_array (result) [val shr 4] := __set_array (result) [val shr 4] or (1 shl (val and 15))
    end;
        

function __set_add_range (minval, maxval: integer; var s: __set_array): __generic_set_type; 
    var
        i: integer;
    begin
        __set_array (result) := s;
        for i := minval to maxval do
            __set_array (result) [i shr 4] := __set_array (result) [i shr 4] or (1 shl (i and 15))
    end;

function __in_set (v: integer; var s:__set_array): boolean;
    begin
        __in_set := s [v shr 4] and (1 shl (v and 15)) <> 0
    end;
    
function __set_union (var s, t: __set_array): __generic_set_type; 
    var
        i: integer;
    begin
        for i := 0 to __set_words - 1 do
            __set_array (result) [i] := s [i] or t [i]
    end;
    
function __set_intersection (var s, t: __set_array): __generic_set_type;
    var
        i: integer;
    begin
        for i := 0 to __set_words - 1do
            __set_array (result) [i] := s [i] and t [i]
    end;
    
function __set_diff (var s, t: __set_array): __generic_set_type;
    var
        i: integer;
    begin
        for i := 0 to __set_words - 1 do
            __set_array (result) [i] := s [i] and not t [i]
    end;

function __set_equal (var s, t: __set_array): boolean;
    begin
        __set_equal := compareWord (s, t, __set_words)
    end;
    
function __set_not_equal (var s, t: __set_array): boolean;
    begin
        __set_not_equal := not compareWord (s, t, __set_words)
    end;
    
function __set_sub (var s, t: __set_array): boolean;
    begin
        __set_sub := __set_super (t, s)
    end;
    
function __set_super (var s, t: __set_array): boolean;
    var
        i: integer;
    begin
        for i := 0 to __set_words do
            if s [i] and t [i] <> t [i] then
                begin	// TODO: implement exit (false)
                    __set_super := false;
                    exit
                end;
        __set_super := true
    end;
    
function __set_sub_not_equal (var s, t: __set_array): boolean;
    begin
        __set_sub_not_equal := __set_sub (s, t) and not __set_equal (s, t)
    end;
    
function __set_super_not_equal (var s, t: __set_array): boolean; 
    begin
        __set_super_not_equal := __set_super (s, t) and not __set_equal (s, t)
    end;

begin
    loadCharset;
    gotoxy (0, 0);
end.