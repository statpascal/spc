program absolutetest;

var
    a: integer;
    b: byte absolute a;
    c: char absolute b;

type
    indextype = 1..sizeof (a);

var
    d: array [indextype] of byte absolute a;
    i: indextype;

procedure test (a: integer);
    var 
	b: word absolute a;
    begin
        writeln (b);
    end;

begin
    a := 65;
    writeln (a, ' ', b, ' ', c);
    for i := 1 to sizeof (a) do
        write (d [i]:4);
    writeln;
    test (50000);
end.
