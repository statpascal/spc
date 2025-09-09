program readtest;

var
    s: string;
    n: integer;
    ch: char;
    
begin
    write ('String: ');
    readln (s);
    writeln ('Got:    ', s);
    
    write ('Int: ');
    readln (n);
    writeln ('Got: ', n);
    
    write ('Char: ');
    readln (ch);
    writeln ('Got:  ', ch);    
    
    while keypressed do;
    waitkey
end.