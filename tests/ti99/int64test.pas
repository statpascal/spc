program int64test;

type
    bytearray = array [0..7] of uint8;

var
    ch: char;
    a, b, c: uint64;
    
const
    c1: bytearray = ($ff, $70, $07, $00, $01, $80, $10, $08);
    c2: bytearray = ($0f, $c0, $01, $08, $01, $40, $30, $07);
    
procedure print (a: uint64);
    var
        i: 0..7;
    begin
        for i := 0 to 7 do
            write (hexstr2 (bytearray (a) [i]));
        writeln
    end;
    
begin
    b := uint64 (c1);
    c := uint64 (c2);

    a := b or c;
    print (a);
    a := b and c;
    print (a);
    a := not b;
    print (a);
    a := b and not c;
    print (a);
    a := b xor c;
    print (a);    
    
    writeln (b = c);
    writeln (b <> c);
    writeln (b < c);
    writeln (b <= c);
    writeln (b > c);
    writeln (b >= c);
    waitkey
end.
