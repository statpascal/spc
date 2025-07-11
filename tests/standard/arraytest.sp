program arraytest;

const 
    n = 5;
    offset = 1 * 1000 * 1000; 

var
    a: array [0..n] of byte;
    i: int64;

    b: array [-n..n] of int64;
    j: -n..n;

    c: array [offset..offset + n] of int64;
    k: offset..offset + n;

    d: array [-n..n, -n..n] of -n * n..n * n;

begin
    for i := 1 to n do 
	a [i] := 2 * i;
    for i := 1 to n do
	write (a [i]:5);
    writeln;

    for j := -n to n do
	b [j] := j + 2;
    for j := -n to n do
	write (b [j]:5);
    writeln;

    for k := offset to offset + n do
        c [k] := 2 * k;
    for k := offset to offset + n do
	write (c [k]:12);
    writeln;

    for i := -n to n do
	for j := -n to n do
	    d [i, j] := i * j;
    for i := -n to n do
	begin
  	    for j := -n to n do
		write (d [i][j]:5);
	    writeln
	end;
    writeln


end.
