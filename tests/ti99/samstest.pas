program samstest;

uses vdp;

const 
    SamsCRUAddr =   $1e00;
    SamsRegs =   16;
    SamsPageSize =   4096;

function swap (n: integer): integer;
    // TODO:  make compiler intrinsic
    begin
        swap := (n shr 8) or (n and $ff) shl 8
    end;

procedure samsCardOn;
    var 
        cruAddr: integer;
    begin
        for cruAddr := $10 to $1f do
            setCRUBit (cruAddr shl 8, false);
        setCRUBit (SamsCRUAddr, true);
    end;

var
    reg: 0..15;

begin
    samsCardOn;
    for reg := 0 to pred (SamsRegs) do 
        memW [$4000 div 2 + reg] := swap (reg);
        
    writeln ('Reading back SAMS regs:');
    for reg := 0 to pred (SamsRegs) do
        writeln ('Reg: ', reg:2, ' byte: ', memB [$4000 + 2 *reg]:2, ' word: ',  memW [$4000 div 2 + reg]:4);

    setCRUBit (SamsCRUAddr, false);
    waitkey
end.