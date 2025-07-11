program strtest;

var 
    s: string;
    a: integer;
    b: single;

begin
    a := 12345;
    b := exp (1);
    str (a, s);
    writeln (s);
    str (a:10, s);
    writeln (s);
    
    str (b:10:8, s);
    writeln (s)
end.
