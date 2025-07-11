program test2;

var
    a: integer;
    p: ^integer;

begin
    p := addr (a);
    p^ := 5;
    writeln (a)
end.
