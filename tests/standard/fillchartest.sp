program fillchartest;

const 
    n = 10;

var
    s: string;
    a: int64;
    b: array [1..n] of boolean;
    i: 1..n;
 

begin
    fillchar (a, sizeof (a), $7f);
    fillchar (a, sizeof (a) - 1, $ff);
    writeln (a);

    s := 'Hello, world';
    fillchar (s [pos ('w', s)], 5, 'X');
    writeln (s);

    fillchar (b, sizeof (b), true);
    fillchar (b, sizeof (b) div 2, false);
    for i := 1 to sizeof (b) do
	write (b [i]:6);
    writeln
end.
