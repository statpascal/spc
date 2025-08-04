program listdsr;

type
    TStandardHeaderNodePtr = ^TStandardHeaderNode;
    TStandardHeaderNode = record
        next: TStandardHeaderNodePtr;
        address: integer;
        name: string
    end;
    
    TStandardHeader = record
        magic, version, nrprogs, filler: uint8;
        pwrupList: pointer;
        prgList, dsrList, subList: TStandardHeaderNodePtr;
        isrList: pointer
    end;

var
    header: TStandardHeader absolute $4000;
    dsrList: TStandardHeaderNodePtr;
    cruAddr: integer;
    
begin
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
