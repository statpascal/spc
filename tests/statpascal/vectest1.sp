program vectest1;

const
    n = 1; // 100 * 1000 * 1000;
    m = 10;

var
    a, b, c: vector of real;
    d: vector of int64;
    i, retval: integer;
    s: string;

begin
    a := intvec (1, m);
    b := intvec (1, m);
    for i := 1 to n do
	c := a + b;
    writeln (c);
    writeln (sum (intvec (1, 100)));
    writeln (sum (realvec (1, 1000, 1)));
    writeln (count (intvec (1, 100) <= 50));
    writeln (cumsum (intvec (1, 20)));
    writeln (cumsum (realvec (0, 1, 0.1)));
    writeln (rev (intvec (1, 5)));
    d := intvec (1, 10);
    writeln (rev (d));
    writeln (d);
    writeln (rev (realvec (0, 1, 0.1)));
    d := randomperm (10);
    writeln (sort (d))
end.
