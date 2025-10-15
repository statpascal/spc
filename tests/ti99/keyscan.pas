program keyscan;

uses vdp;

type
    TKeyboardData = array [0..7] of uint8;
    
procedure readKeyboard (var data: TKeyboardData); assembler;
        mov @data, r14
        clr r13
    readKeyboard_1:
        li r12, >0024	// select keyboard column (0-7)
        ldcr r13, 3
        
        li r12, >0006	// read rows and store in data
        stcr *r14+, 8
    
        ai r13, >0100
        ci r13, >0800
        jne readKeyboard_1
end;

function readGrom (address: integer): uint8;
    begin
        gromwa := char (address shr 8);
        gromwa := char (address and 255);
        readGrom := uint8 (gromrd)
    end;
    
function mapKey (var keys: TKeyboardData): char;
    var
        gromAddr: integer;
        col, row: integer;
        ch: char;
    begin
        if (keys [0] and 16 = 0) then
            gromAddr := $1760
        else if (keys [0] and 32 = 0) then
            gromAddr := $1730
        else if (keys [0] and 64 = 0) then
            gromAddr := $1790
        else
            gromAddr := $1700;
        for col := 0 to 5 do
            for row := 0 to 7 do
                if keys [col] and (1 shl row) = 0 then
                    begin
                        ch := chr (readGrom (gromAddr + 8 * col + (7 - row)));
                        if ch <> #$ff then
                            begin
                                mapKey := ch;
                                exit
                            end
                    end;
        mapKey := #0
    end;
    
var
    keys: TKeyboardData;
    i: integer;
    ch: char;
    
begin
    repeat
        readKeyboard (keys);
        gotoxy (0, 0);
        for i := 0 to 7 do
            writeln ('Col ', i, ': ', hexstr2 (keys [i]):3);
        writeln;
        
        ch := mapKey (keys);
        if ch <> #0 then 
            if ch in [#33..#127] then
                write ('Key: ', ch)
            else
                write ('Key: #', ord (ch))
        else 
            write (' ': 10)
    until false
end.        