unit system;

interface

type
    integer = int16;
    
procedure gotoxy (x, y: integer);
procedure writeint (a: integer);
procedure writechar (ch: char);

procedure waitkey; external;
    
implementation

const
    WriteAddr = $4000;

var 
    vdpWriteAddress: integer;
    vdprd, vdpsta, vdpwd, vdpwa: ^char;
    
procedure setVdpAddress (n: integer);
    begin
        vdpwa^ := chr (n and 255);
        vdpwa^ := chr (n shr 8)
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
                        p^ := vdprd^;
                        inc (p)
                    end;
                p := addr (line);
                setVdpAddress (pred (i) shl 5 or WriteAddr);
                while p <> q do
                    begin
                        vdpwd^ := p^;
                        inc (p)
                end;
            end;
        for i := 0 to 31 do
            vdpwd^ := chr (32);
        dec (vdpWriteAddress, 32);
        setVdpAddress (vdpWriteAddress)
    end;
    
    
procedure gotoxy (x, y: integer);
    begin
        vdpWriteAddress := y shl 5 + x;
        setVdpAddress (vdpWriteAddress or WriteAddr)
    end;
    
procedure writeint (a: integer);
    begin
        if a >= 10 then
            writeint (a div 10);
        writechar (chr (a mod 10 + 48))
    end;
    
procedure writechar (ch: char);
    begin
        vdpwd^ := ch;
        inc (vdpWriteAddress);
        if vdpWriteAddress = 24 * 32 then
            scroll;
    end;

begin
    integer (vdprd) := $8800;
    integer (vdpsta) := $8802;
    integer (vdpwd) := $8c00;
    integer (vdpwa) := $8c02;
    gotoxy (0, 0)
end.
