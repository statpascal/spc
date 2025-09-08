program manyfiles;

const
    nFiles = 20;

var
    f: array [1..nFiles] of text;
    i: integer;
    s: string;
    
begin
    for i := 1 to nFiles do
        begin
            str (i, s);
            assign (f [i], 'DSK1.TEST' + s + '.TXT')
        end;
    for i := 1 to nFiles do
        begin
            rewrite (f [i]);
            writeln (f [i], 'This is file TEST', i, '.TXT');
            close (f [i])
        end;
    for i := 1 to nFiles do
        begin
            reset (f [i]);
            while not eof (f [i]) do
                begin
                    readln (f [i], s);
                    writeln (i:3, ': ', s)
                end;
            close (f [i])
        end;
        
    waitkey
end.    
    