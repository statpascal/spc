program newtest;

var
    a: ^integer;

begin
    new (a);
    a^ := 5;
    writeln (a^);
    dispose (a)
end.
