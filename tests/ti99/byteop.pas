program byteop;

type
    tinyint = 0..10;

var 
    i,  j: tinyint;
    
begin
    for i := 1 to 10 do
        for j := 1 to 10 do
            begin
                gotoxy (3 * pred (i), pred (j));
                writeint (i * j)
            end;
    waitkey
end.
