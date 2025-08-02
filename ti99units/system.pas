unit system;

interface

type
    integer = int16;
    PChar = ^char;
    string1 = string [1];
    
var
    input, output: text;
    
procedure gotoxy (x, y: integer);

procedure move (var src, dest; length: integer); external;

function min (a, b: integer): integer;
function max (a, b: integer): integer;
function sqr (a: integer): integer;

procedure __write_lf (var f: text);
procedure __write_int (var f: text; n, length, precision: integer);
procedure __write_char (var f: text; ch: char; length, precision: integer);
procedure __write_string (var f: text; p: PChar; length, precision: integer);
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);

function __short_str_char (ch: char): string1;
function __short_str_concat (a, b: PChar): string;
function length (s: PChar): integer;

procedure waitkey; external;
    
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
        setVdpAddress (vdpWriteAddress or WriteAddr)
    end;
    
procedure writechar (ch: char);
    begin
        vdpwd := ch;
        inc (vdpWriteAddress);
        if vdpWriteAddress = 24 * 32 then
            scroll
    end;
    
procedure __write_lf (var f: text);
    begin
        vdpWriteAddress := (vdpWriteAddress + 32) and not 31;
        if vdpWriteAddress = 24 * 32 then
            scroll
        else 
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
        i: integer;
    begin
        for i := 1 to pred (length) do
            writechar (' ');
        writechar (ch)
    end;
    
procedure __write_string (var f: text; p: PChar; length, precision: integer);
    var
        len, i: integer;
    begin
        len := ord (p^);
        while vdpWriteAddress + length > 24 * 32 do
            scroll;
        inc (vdpWriteAddress, max (len, length));
        for i := len + 1 to length do
            vdpwd := ' ';
        while len > 0 do 
            begin
                inc (p);
                vdpwd := p^;
                dec (len)
            end;
    end;
    
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);
    begin
        if b then
            __write_string (f, 'true', length, precision)
        else
            __write_string (f, 'false', length, precision)
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

function __short_str_concat (a, b: PChar): string;
    var
        res: PChar;
    begin
        res := PPChar (addr (a) + (-1))^;	// addr of result is on stack before a
        move (a^, res^, ord (a^) + 1);
        move (b [1], res [ord (a^) + 1], min (ord (b^), 255 - ord (a^)));
        res^ := chr (min (255, ord (a^) + ord (b^)))
    end;
    
function length (s: PChar): integer;
    begin
        length := ord (s^)
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
