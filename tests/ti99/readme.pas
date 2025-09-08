program readme;

var
    s: string;
    
begin
    close (input);
    assign (input, 'DSK1.readme.md');
    reset (input);
    
    while not eof do
        begin
            readln (s);
            writeln (s)
        end;
    close (input);
    
    writeln;
    writeln ('Now copying readme.md to TEXT.TXT');
    
    close (output);
    assign (output, 'DSK1.TEST.TXT');
    reset (input);
    rewrite (output);
    
    while not eof do
        begin
            readln (s);
            writeln (s)
        end;
        
    close (input);
    close (output);
    
    assign (output, '');
    rewrite (output);
    writeln ('Copy done');
    
    waitkey
end.
