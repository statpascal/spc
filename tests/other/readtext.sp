program readtext;

var
    s: string;
    count: cardinal;
    
begin
    count := 0;
    while not eof do
        begin
            count := count + 1;
            readln (s);
            writeln (count:5, ' ', s)
        end
end.