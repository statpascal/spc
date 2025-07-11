program static;
uses staticunit;

procedure count;
    const
	n: integer = 5;
    begin
	n := n + 1;
	writeln ('n = ', n)
    end;

procedure test;
    type
	enum = (val1, val2, val3, val4);
	rec1 = record
	    a: integer;
	    s: string
	end;
	rec2 = record
	    a: real;
	    b: array [1..2] of rec1
	end;
    const 
	n = 5;
	m = 3;
 	a: array [1..n] of integer = (10, 12, 14, 16, 18);
        mat: array [1..m, 1..m] of real = ((1, 0, 0), (0, 1, 0), (0, 0, 1));
	s: array [1..n] of string = ('String 1', 'String 2', 'String 3', 'String 4', 'String 5');
	e: array [enum] of enum = (val2, val4, val1, val3);
	r1: rec1 = (a: 5; s: 'ABCD');
	r2: rec2 = (a: 3.1415; b: ((a: 7; s: 'A'), (a: 8; s: 'B')));
    var
	i: 1..n;
	j, k: 1..m;
        l: enum;
    begin
	for i := 1 to n do
	    write ('a [', i, '] = ', a [i], '  ');
	writeln;
	for j := 1 to m do 
	    begin
	        for k := 1 to m do
		    write (mat [j, k]:4:1);
		writeln
	    end;
	for i := 1 to n do
	    writeln ('s [', i, '] = ', s [i]);
	for l := val1 to val4 do
	    write (ord (e [l]):3);
	writeln;
        writeln ('r1 = ', r1.a, ', ', r1.s);
        writeln ('r2 = ', r2.a:10:7, ', ', r2.b [1].a, ', ', r2.b [1].s, ', ', r2.b [2].a, ', ', r2.b [2].s)
    end;

begin
    count;
    count;
    writeln ('b = ', b:10:5);
    test;
end.
