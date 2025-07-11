program getfreemem;

type
    field = array [1..maxint] of char;

var
    s: string;
    p: pointer;
    q: ^field;
    i, n: integer;

begin
    s := 'Hello, world';
    n := length (s) + 1;
    getmem (p, n);
    move (s [1], p^, n);
    getmem (q, n);
    move (p^, q^, n);
    for i := 1 to n - 1 do
	write (q^[i]);
    writeln;
    freemem (q, n);
    freemem (p, n)
end.
