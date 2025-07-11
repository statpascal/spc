program movetest;

const
    n = 10;

var
    a, b: int64;
    x, y: array [1..n] of real;
    i: 1..n;

begin
    a := 12345;
    move (a, b, sizeof (a));
    writeln (b);
    for i := 1 to n do
	x [i] := exp (i);
    move (x, y, n * sizeof (real));
    for i := 1 to n do
	write (y [i]:15:8);
    writeln
end.

    