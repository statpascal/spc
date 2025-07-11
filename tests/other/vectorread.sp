program dummy;

var 
    txt: vector of string;
    s: string;
    i: integer;

begin
    while not eof do 
        begin
            readln (s);
            txt := combine (txt, s)
        end;

    for i := 1 to size (txt) do
        writeln (i:5, '  ', txt [i])
end.