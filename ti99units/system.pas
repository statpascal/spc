unit system;

interface

type
    integer = int16;
    PChar = ^char;
    string1 = string [1];
    string4 = string [4];
    
var
    input, output: text;
    memb: array [0..65535] of uint8 absolute $0000;
    memw: array [0..32767] of integer absolute $0000;
    
procedure gotoxy (x, y: integer);

procedure move (var src, dest; length: integer); external;
procedure vmbw (var src; dest, length: integer); external;
procedure vmbr (var dest; src, length: integer); external;

function min (a, b: integer): integer;
function max (a, b: integer): integer;
function sqr (a: integer): integer;

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

procedure waitkey; external;

procedure __new (var p: pointer; count, size: integer);
procedure __dispose (p: pointer);
procedure mark (var p: pointer);
procedure release (p: pointer);

procedure setCRUBit (addr: integer; val: boolean); external;

function hexstr (n: integer): string4;

    
implementation

const
    WriteAddr = $4000;

type
    PPChar = ^PChar;
var 
    vdpWriteAddress: integer;
    vdprd:  char absolute $8800;
    vdpsta: char absolute $8802;
    vdpwd:  char absolute $8c00;
    vdpwa:  char absolute $8c02;
    gromwa: char absolute $9c02;
    gromrd: char absolute $9800;
    
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
    
procedure setVdpAddress (n: integer);
    begin
        vdpwa := chr (n and 255);
        vdpwa := chr (n shr 8)
    end;
    
procedure _rt_scroll_up; external;    

procedure gotoxy (x, y: integer);
    begin
        vdpWriteAddress := y shl 5 + x;
        setVdpAddress (vdpWriteAddress or WriteAddr)
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
    var
        buf: string [6];
        neg: boolean;
        p: PChar;
    begin
        if n = -32768 then
            __write_string (f, '-32768', length, precision)
        else
            begin
                p := addr (buf [6]);
                neg := n < 0;
                if neg then
                    n := -n;
                repeat
                    p^ := char (n mod 10 + 48);
                    dec (p);
                    if n > 0 then
                        n := n div 10
                until n = 0;
                if neg then
                    begin
                        p^ := '-';
                        dec (p)
                    end;
                p^ := chr (integer (addr (buf [6])) - integer (p));
                __write_string (f, p, length, precision)
            end
    end;
    
procedure __write_char (var f: text; ch: char; length, precision: integer);
    var
        s: string [1];
    begin
        s [0] := #1;
        s [1] := ch;
        __write_string (f, s, length, precision);
    end;
    
procedure __write_string (var f: text; p: PChar; length, precision: integer);
    var
        strlen, len, i: integer;
    begin
        strlen := ord (p^);
        len := max (strlen, length);
        while vdpWriteAddress + len > 24 * 32 do
            scroll;
        setVdpAddress (vdpWriteAddress or WriteAddr);
        inc (vdpWriteAddress, len);
        for i := strlen + 1 to length do
            vdpwd := ' ';
        while strlen > 0 do 
            begin
                inc (p);
                vdpwd := p^;
                dec (strlen)
            end;
    end;
    
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);
    const
        s: array [boolean] of string [5] = ('false', 'true');
    begin
        __write_string (f, s [b], length, precision)
    end;

function min (a, b: integer): integer;
    begin
        if a < b then
            min := a
        else
            min := b
    end;

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
    var
        res: PChar;
    begin
        res := PPChar (addr (ch) + (-1))^;	// addr of result is on stack before a
        res [0] := #1;
        res [1] := ch
    end;

// TODO: pass max. length of result string - this will crash if too short

function __short_str_concat (a, b: PChar): string;
    var
        res: PChar;
    begin
        res := PPChar (addr (a) + (-1))^;	// addr of result is on stack before a
        move (a^, res^, ord (a^) + 1);
        move (b [1], res [ord (a^) + 1], min (ord (b^), 255 - ord (a^)));
        res^ := chr (min (255, ord (a^) + ord (b^)))
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
    var
        res: PChar;
    begin
        res := PPChar (addr (s) + (-1))^;	// addr of result is on stack before a
        if start <= ord (s^) then
            begin
                if start + len > succ (ord (s^)) then
                    len := succ (ord (s^) - start);
                move (s [start], res [1], len);
                res^ := chr (len)
            end
        else
            res^ := #0
    end;

function hexstr (n: integer): string4;
    const 
        hex: string [16] = '0123456789ABCDEF';
    var
        res: string [4];
        i, j: integer;
    begin
        for i := 1 to 4 do
            res [i] := hex [succ (n shr (16 - i shl 2) and $0f)];
        res [0] := #4;
        hexstr := res
    end;
    
procedure loadCharset;
    var 
        i, j: integer;
    begin
        gromwa := #$06;	// >06b4: standard char set
        gromwa := #$b4;
        setVdpAddress ($0800 + 8 * 32 or WriteAddr);
        for i := 32 to 127 do
            begin
                for j := 1 to 7 do
                    vdpwd := gromrd;
                vdpwd := chr (0)
            end
    end;

begin
    loadCharset;
    gotoxy (0, 0)
end.
