program byteop;

type
    tinyint = integer;

var 
    i,  j: 0..10;
    
begin
    for i := 1 to 10 do
        for j := 1 to 10 do
            begin
                gotoxy (3 * pred (i), pred (j));
                writeint (i * j)
            end;
    waitkey
end.
