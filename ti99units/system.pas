unit system;

interface

type
    integer = int16;
    string = array [0..80] of char;
    
    PChar = ^char;
    
var
    input, output: text;
    __globalruntimedata: pointer;
    
procedure gotoxy (x, y: integer);

procedure __write_lf (var f: text; runtimeData: pointer); 
procedure __write_int64 (var f: text; n, length, precision: integer; runtimeData: pointer); 
procedure __write_char (var f: text; ch: char; length, precision: integer; runtimeData: pointer);
procedure __write_string (var f: text; p: PChar; length, precision: integer; runtimeData: pointer);
//procedure __write_boolean (var f: text; b: boolean; length, precision: integer; runtimeData: pointer);

procedure waitkey; external;
    
implementation

const
    WriteAddr = $4000;

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

procedure scroll;
    var    
        line: array [0..31] of char;
        i: integer;
        p, q: ^char;
    begin
        q := addr (line) + 32;
        for i := 1 to 23 do 
            begin
                setVdpAddress (i shl 5);
                p := addr (line);
                while p <> q do
                    begin
                        p^ := vdprd;
                        inc (p)
                    end;
                p := addr (line);
                setVdpAddress (pred (i) shl 5 or WriteAddr);
                while p <> q do
                    begin
                        vdpwd := p^;
                        inc (p)
                end;
            end;
        for i := 0 to 31 do
            vdpwd := chr (32);
        dec (vdpWriteAddress, 32);
        setVdpAddress (vdpWriteAddress or WriteAddr)
    end;
    
    
procedure gotoxy (x, y: integer);
    begin
        vdpWriteAddress := y shl 5 + x;
        setVdpAddress (vdpWriteAddress or WriteAddr)
    end;
    
procedure writechar (ch: char);
    begin
        vdpwd := ch;
        inc (vdpWriteAddress);
        if vdpWriteAddress = 24 * 32 then
            scroll
    end;
    
procedure writeint (a, len: integer);
    var
        i: integer;
    begin
        if a < 0 then
            begin
                writechar ('-');
                writeint (-a, pred (len))
            end
        else
            begin
                if a >= 10 then
                    writeint (a div 10, pred (len))
                else
                    for i := 2 to len do 
                        writechar (' ');
                writechar (chr (a mod 10 + 48))
            end
    end;
    
procedure __write_lf (var f: text; runtimeData: pointer); 
    begin
        vdpWriteAddress := (vdpWriteAddress + 32) and not 31;
        if vdpWriteAddress = 24 * 32 then
            scroll
        else 
            setVdpAddress (vdpWriteAddress or WriteAddr);
    end;
    
procedure __write_int64 (var f: text; n, length, precision: integer; runtimeData: pointer); 
    begin
        writeint (n, length)
    end;
    
procedure __write_char (var f: text; ch: char; length, precision: integer; runtimeData: pointer);
    var
        i: integer;
    begin
        for i := 1 to pred (length) do
            writechar (' ');
        writechar (ch)
    end;
    
procedure __write_string (var f: text; p: PChar; length, precision: integer; runtimeData: pointer);
    var
        len: integer;
    begin
        len := ord (p^);
        while len > 0 do 
            begin
                inc (p);
                writechar (p^);
                dec (len)
            end
    end;
    
procedure loadCharset;
    var 
        i, j: integer;
    begin
        gromwa := #$06;	// >06b0: standard char set
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
