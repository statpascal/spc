program pointertest;

var
    a: ^integer;
    b: ^word;
    c: pointer;

begin
    new (a);
    a^ := 65537;
    c := a;
    b := c;
    writeln (b^);
    dispose (a)
end.