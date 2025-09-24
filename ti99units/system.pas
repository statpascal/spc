unit system;

interface

const
    MaxInt = $7fff;

type
    shortint = int8;
    byte = uint8;
    integer = int16;
    
    PChar = ^char;
    string1 = string [1];
    string2 = string [2];
    string4 = string [4];
    
    TVdpRegList = array [0..7] of uint8;
    
    TPab = record
        opcode, err_type: uint8;
        vdpaddr: integer;
        reclen, numchar: uint8;
        recnr: integer;
        status: uint8;
        name: string [25]	// TODO:  maximum file name length?
    end;

    __file_data = record
        fileidx: integer;
        bufpos: integer;
        pab: TPab
    end;
    
    __bin_file_type = file of void;
    text = file of void;
    
var
    input, output: text;
    memb: array [0..65535] of uint8 absolute $0000;
    memw: array [0..32767] of integer absolute $0000;
    
// Screen and keyboard access
    
const
    InvalidKey = #$ff;
    
function getKey: char;

procedure gotoxy (x, y: integer);
function whereX: integer;
function whereY: integer;

procedure clrscr;

procedure vmbw (var src; dest, length: integer);	// video multiple byte write
procedure vmbr (var dest; src, length: integer);	// video multiple byte read
procedure vrbw (val: uint8; length: integer);		// video repeated byte write
procedure writeV (addr: integer; val: uint8);
function readV (addr: integer): uint8;

// Utilites

procedure move (var src, dest; length: integer);
function compareWord (var src, dest; length: integer): boolean;
procedure fillChar (var dest; count: integer; value: char);
procedure fillChar (var dest; count: integer; value: uint8);

function min (a, b: integer): integer; intrinsic name '__min';
function max (a, b: integer): integer; intrinsic name '__max';
function sqr (a: integer): integer; intrinsic name '__sqr';
function abs (n: integer): integer; intrinsic name '__abs';

// file handling

procedure __write_lf (var f: text);
procedure __write_int (var f: text; n, length, precision: integer);
procedure __write_char (var f: text; ch: char; length, precision: integer);
procedure __write_string (var f: text; p: PChar; length, precision: integer);
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);

procedure __assign (var f; filename: string);

procedure __rewrite_text (var f: text);
procedure __reset_text (var f: text);

procedure __rewrite_bin (var f; blocksize: integer);
procedure __reset_bin (var f; blocksize: integer);

procedure __write_bin_typed (var f, data);
procedure __write_bin_ign (var f; var data; blocks: integer);
procedure __seek_bin (var f; pos: integer);

procedure __read_bin_typed (var f, data);
procedure __read_bin_ign (var f; var data; blocks: integer);

procedure __close (var f);
procedure __erase (var f);

function __eof (var f): boolean;
function __eof_input: boolean;

procedure __end_line (var f: text);

function __read_int (var f: text): integer;
function __read_char (var f: text): char;
function __read_string (var f: text): string;
procedure __read_lf (var f: text);

const
    TooManyOpenFiles = 1;
    FileOpenFailed = 2;
    InvalidFileName = 3;
    FileNotOpen = 4;

var
   InOutRes: integer;
   
function IOResult: integer;

// string handling

function length (s: PChar): integer;
function pos (ch: char; s: PChar): integer;
function copy (s: PChar; start, len: integer): string;

procedure __str_int (n, length, precision: integer; s: PChar);

function __short_str_char (ch: char): string1;
function __short_str_concat (a, b: PChar): string;
function __short_str_equal (s, t: PChar): boolean;
function __short_str_not_equal (s, t: PChar): boolean; 
function __short_str_less_equal (s, t: PChar): boolean; 
function __short_str_greater_equal (s, t: PChar): boolean; 
function __short_str_less (s, t: PChar): boolean; 
function __short_str_greater (s, t: PChar): boolean; 

function __copy_str_const (var s: string): string; external;

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

// Set routines

const
    __set_words = 16;

type
    __generic_set_type = set of 0..255;
    __set_array = array [0..__set_words - 1] of integer;
    
function __set_add_val (val: integer; var s: __set_array): __generic_set_type; 
function __set_add_range (minval, maxval: integer; var s: __set_array): __generic_set_type; 

function __copy_set_const (var s: __generic_set_type): __generic_set_type; external;

function __in_set (v: integer; var s: __set_array): boolean; intrinsic;
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

uses dsr;

// simple UCSD-like heap managment in lower memory

const
    heapPtr: integer = $2000;
    heapMax = $4000;
    
procedure __new (var p: pointer; count, size: integer);
    var
        n: integer;
    begin
        n := (count * size + 1) and not 1;
        if heapPtr + n > heapMax then
            p := nil
        else 
            begin
                p := pointer (heapPtr);
                inc (heapPtr, n);
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

var
    vdpWriteAddress: integer;
    
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
        li r12, >8320
        
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
        li r12, >8320
        
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
        jmp comparebyte_4		// TODO: add rt with near/far return
        
    comparebyte_3:
        clr r12
        movb r12, *r15
        
    comparebyte_4:
end;

procedure fillChar (var dest; count: integer; value: char); assembler;
        mov @count, r12
        jeq fillchar_2
        
        mov @dest, r13
        movb @value, r14
    fillchar_1:
        movb r14, *r13+
        dec r12
        jne fillchar_1
        
    fillchar_2:
end;

procedure fillChar (var dest; count: integer; value: uint8);
    begin
        fillChar (dest, count, chr (value))
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

function getKey: char;
    var
        keyboardMode: byte absolute $8374;
        keyPressed: byte absolute $8375;
        gplStatus: byte absolute $837c;
        
    procedure scanKey; assembler;
           lwpi >83e0
           bl @>000e
           lwpi >8300
    end;

    begin
        keyboardMode := 4;
        scanKey;
        if gplStatus and $20 <> 0 then
            getKey := chr (keyPressed)
        else
            getKey := InvalidKey
    end;
    
function __read_line_console: string;
    const
        FctnS = #136;
        Enter = #13;
    var
        row, col, pos, count: integer;
        ch: char;
    begin
        col := whereX;
        row := whereY;
        result [0] := #0;
        pos := 1;
        count := 0;
        repeat
            ch := getKey;
            case ch of
                FctnS:
                    if pos > 1 then
                        begin
                            dec (pos);
                            dec (result [0]);
                            gotoxy (col, row);
                            write (result, ' ');
                            if col + pos < 31 then write (' ');
                            count := 0
                        end;
                #32..#127:
                    begin
                        result [pos] := ch;
                        if col + pos < 31 then
                            begin
                                inc (pos);
                                inc (result [0])
                            end;
                        gotoxy (col, row);
                        write (result);
                        count := 0
                    end;
                Enter:
                    count := $100;	// turn off cursor
            end;
            inc (count);
            if count and $ff = 1 then
                begin
                    gotoxy (col + pos - 1, row);
                    if odd (count shr 8) then write (' ') else write (#$1f)
                end;
        until ch = Enter;
        writeln;
    end;


    
procedure gotoxy (x, y: integer);
    begin
        vdpWriteAddress := y shl 5 + x;
        setVdpAddress (vdpWriteAddress or WriteAddr)
    end;
    
function whereX: integer;
    begin
        whereX := vdpWriteAddress and $1f
    end;
    
function whereY: integer;
    begin
        whereY := vdpWriteAddress shr 5
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
        if f.fileidx = 0 then
            begin
                vdpWriteAddress := (vdpWriteAddress + 32) and not 31;
                if vdpWriteAddress = 24 * 32 then
                    scroll;
                setVdpAddress (vdpWriteAddress or WriteAddr);
            end
        else
            __end_line (f)
    end;
    
procedure __write_int (var f: text; n, length, precision: integer);
    var
        buf: string;
    begin
        __str_int (n, length, -1, buf);
        __write_string (f, buf, length, -1);
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

    var
        i, len, outlen: integer;
    begin
        len := ord (p^);
        outlen := max (len, length);
        if f.fileidx = 0 then 
            begin
                while vdpWriteAddress + outlen > 24 * 32 do
                    scroll;
                setVdpAddress (vdpWriteAddress or WriteAddr);
                inc (vdpWriteAddress, outlen);
                if outlen > len then
                    vrbw (32, outlen - len);
                __write_data (p, len)
            end
        else
            begin
                for i := succ (len) to outlen do
                    if f.bufpos < f.pab.reclen then begin
                        writeV (f.pab.vdpaddr + f.bufpos, ord (' '));
                        inc (f.bufpos)
                    end;
                for i := 1 to len do
                    if f.bufpos < f.pab.reclen then begin
                        writeV (f.pab.vdpaddr + f.bufpos, ord (p [i]));
                        inc (f.bufpos)
                    end;
            end
    end;
    
procedure __write_boolean (var f: text; b: boolean; length, precision: integer);
    begin
        if b then
            __write_string (f, 'true', length, -1)
        else
            __write_string (f, 'false', length, -1) 
    end;
    
// file handling

const
    nFiles = 3;
    fileBufSize = 245;
    
type
    TFileDescriptor = record
        isOpen: boolean;
        pabAddr, bufAddr: integer
    end;
    
var
    vdpFree: integer absolute $8370;
    openFiles: array [1..nFiles] of TFileDescriptor;

function IOResult: integer;
    begin
        IOResult := InOutRes;
        InOutRes := 0
    end;
    
procedure reserveVdpBufs;
    var 
        i: integer;
    begin
        for i := 1 to nFiles do
            with openFiles [i] do
                begin
                    dec (vdpFree, fileBufSize);
                    isOpen := false;
                    bufAddr := succ (vdpFree);
                    dec (vdpFree, sizeof (TPab));
                    pabAddr := succ (vdpFree)
                end
    end;
        
procedure __assign (var f; filename: string);
    begin
        
        __file_data (f).pab.name := filename
    end;
    
function getFileDescriptor (var f: __file_data; isTextFile: boolean): boolean;
    var
        i: integer;
    begin
        f.fileidx := -1;
        f.bufpos := 0;
        fillChar (f.pab, 8, 0);
        
        if f.pab.name = '' then
            if isTextFile then
                f.fileidx := 0
            else
                InOutRes := InvalidFileName
        else
            begin
                i := 1;
                repeat
                    with openFiles [i] do
                        if not isOpen then
                            begin
                                isOpen := true;
                                f.fileidx := i;
                                f.pab.vdpaddr := openFiles [f.fileidx].bufAddr
                            end;
                    inc (i)
                until (i > nFiles) or (f.fileidx <> -1)
            end;
        InOutRes := TooManyOpenFiles * ord (f.fileidx = -1);
        getFileDescriptor := InOutRes = 0
    end;
        
function execDsr (var f: __file_data): boolean;
    begin
        execDsr := dsrLink (f.pab, openFiles [f.fileidx].pabAddr)
    end;        

procedure openFile (var f: __file_data; opcode, ftype, reclen: uint8; isTextFile: boolean);
    begin
        if getFileDescriptor (f, isTextFile) and (f.fileidx <> 0) then
            begin
                f.pab.opcode := opcode;
                f.pab.err_type := ftype;
                f.pab.reclen := reclen;
                InOutRes := FileOpenFailed * ord (not execDsr (f))
            end
    end;

procedure __rewrite_bin (var f; blocksize: integer);
    begin
        openFile (__file_data (f), PabOpen, PabOutput or PabInternal or PabFixed or PabRelative, blocksize, false)
    end;                
    
procedure __reset_bin (var f; blocksize: integer);
    begin
        openFile (__file_data (f), PabOpen, PabUpdate or PabInternal or PabFixed or PabRelative, blocksize, false)
    end;                
    
procedure __rewrite_text (var f: text);
    begin
        openFile (__file_data (f), PabOpen, PabOutput or PabDisplay or PabVariable, 254, true)
    end;
    
procedure __reset_text (var f: text);
    begin
        openFile (__file_data (f), PabOpen, PabInput or PabDisplay or PabVariable, 254, true)
    end;
    
procedure __write_bin_ign (var f; var data; blocks: integer);
    type
        arrtype = array [0..MaxInt] of uint8;
    var 
        I: integer;
    begin
        __file_data (f).pab.opcode := PabWrite;
        for i := 0 to pred (blocks) do
            begin
                vmbw (arrtype (data) [i * __file_data (f).pab.reclen], __file_data (f).pab.vdpaddr, __file_data (f).pab.reclen);
                execDsr (__file_data (f))
            end
    end;        
    
procedure __write_bin_typed (var f, data);
    begin
        __write_bin_ign (f, data, 1)
    end;
    
procedure __seek_bin (var f; pos: integer);
    begin
        if __file_data (f).fileidx > 0 then
        __file_data (f).pab.recnr := pos
    end;

procedure __read_bin_ign (var f; var data; blocks: integer);
    type
        arrtype = array [0..MaxInt] of uint8;
    var 
        I: integer;
    begin
        __file_data (f).pab.opcode := PabRead;
        for i := 0 to pred (blocks) do
            begin
                execDsr (__file_data (f));
                vmbr (arrtype (data) [i * __file_data (f).pab.reclen], __file_data (f).pab.vdpaddr, __file_data (f).pab.reclen)
            end
    end;        
    
procedure __read_bin_typed (var f, data);
    begin
        __read_bin_ign (f, data, 1)
    end;

procedure __read_lf (var f: text);
    begin
    end;
    
function __read_int (var f: text): integer;
    var
        s: string;
        i: integer;
        neg: boolean;
    begin
        s := __read_string (f);
        result := 0;
        neg := (length (s) <> 0) and (s [1] = '-');
        i := succ (ord (neg));
        while (i <= length (s)) and (s [i] in ['0'..'9']) do
            begin
                result := result * 10 + ord (s [i]) - ord ('0');
                inc (i)
            end;
        if neg then
            result := -result
    end;
    
function  __read_char (var f: text): char;
    var 
        s: string;
    begin
        s := __read_string (f);
        if length (s) <> 0 then
            __read_char := s [1]
        else
            __read_char := #0
    end;
    
function __read_string (var f: text): string;
    begin
        if f.fileidx = 0 then
            __read_string := __read_line_Console
        else if f.fileidx > 0 then
            begin
                f.pab.opcode := PabRead;
                execDsr (__file_data (f));
                vmbr (result [1], f.pab.vdpaddr, f.pab.numchar);
                result [0] := chr (f.pab.numchar)
            end
        else
            InOutRes := FileNotOpen
    end;    
    
procedure __close (var f);
    begin
        InOutRes := 0;
        with __file_data (f) do
            begin
                if fileidx > 0 then
                    begin
                        pab.opcode := PabClose;
                        execDsr (__file_data (f));
                        openFiles [fileidx].isOpen := false;
                        fileidx := -1
                    end
                else
                    if fileidx < 0 then
                        InOutRes := 1
            end
    end;
    
procedure __erase (var f);
    begin
        if __file_data (f).fileidx > 0 then
            begin
                __file_data (f).pab.opcode := PabDelete;
                execDsr (__file_data (f))
            end
        else
            InOutRes := FileNotOpen
    end;
    
function __eof (var f): boolean;
    begin
        if __file_data (f).fileidx > 0 then
            begin
                __file_data (f).pab.opcode := PabStatus;
                execDsr (__file_data (f));
                __eof := __file_data (f).pab.status and $01 <> 0;
            end
        else
            InOutRes := FileNotOpen
    end;
    
function __eof_input: boolean;
    begin
        __eof_input := eof (input)
    end;

procedure __end_line (var f: text);
    begin
        if f.fileidx > 0 then    
            begin
                f.pab.opcode := PabWrite;
                f.pab.numchar := f.bufpos;
                execDsr (__file_data (f));
                f.bufpos := 0
            end
    end;

    
// string handling

function __short_str_char (ch: char): string1;
    begin
        result [0] := #1;
        result [1] := ch
    end;

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
    
procedure __str_int (n, length, precision: integer; s: PChar);

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
        res: string [6];
        p: PChar;
    
    begin
        if n = -32768 then
            begin
                res := '-32768';
                p := addr (res [0])
            end
        else
            begin
                p := outint (abs (n), n < 0, addr (res [6]));
                p^ := chr (integer (addr (res [6])) - integer (p))
            end;
        if length > ord (p^) then
            begin
                s [0] := chr (length);
                fillChar (s [1], length - ord (p^), ' ');
                move (p [1], s [succ (ord (s [0]) - ord (p^))], ord (p^))
            end
        else
            move (p^, s^, succ (ord (p^)))
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
        setVdpAddress ($0800 + 8 * 31 or WriteAddr);
        for i := 1 to 7 do
            vdpwd := #$3f;
        vdpwd := #0;
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
        for i := 0 to __set_words - 1 do
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
        for i := 0 to __set_words - 1 do
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
    output.fileidx := 0;
    input.fileidx := 0;
    loadCharset;
    gotoxy (0, 0);
    reserveVdpBufs
end.