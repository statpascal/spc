program listdsr;

uses vdp, dsr;

var
    header: TStandardHeader absolute $4000;
    dsrList: TStandardHeaderNodePtr;
    cruAddr: integer;
    
begin
    // make sure all cards are turned off
    for cruAddr := $10 to $1f do
        setCRUBit (cruAddr shl 8, false);
        
    cruAddr := $1000;
    repeat
        setCRUBit (cruAddr, true);
        if header.magic = $aa then
            begin
                writeln ('CRU base: ', hexstr (cruAddr));
                dsrList := header.dsrList;
                while dsrList <> nil do
                    begin
                        writeln ('  ', hexstr (dsrList^.address), ': ', dsrList^.name);
                        dsrList := dsrList^.next
                    end
            end;
        setCRUBit (cruAddr, false);
        inc (cruAddr, $100)
    until cruAddr = $2000;
    waitkey
end.
