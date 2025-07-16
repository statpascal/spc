program newtest;

type
    rec = record
	a: array [1..100] of integer;
        s: array [1..100] of string
    end;

var
    a: ^integer;
    b: ^rec;

begin
    new (a);
    a^ := 5;
    writeln (a^);
    dispose (a);

    new (b);
    b^.s [50] := 'abcde';
    writeln (b^.s [50]);
    dispose (b)
end.
