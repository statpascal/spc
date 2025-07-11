program settest;

const
    n = 20;

type
    intset = set of 1 ..n;

procedure print (a: intset);
    var
	i: 1..n;
    begin
        for i := 1 to n do
            if i in a then
	        write (i, ' ');
	writeln
    end;

procedure test;
    var
	a, b, c: intset;
    begin
	a := [1..5, 10, 12, 14];
        b := [4, 5, 10..20];
	c := a + b;
	print (c);
	print (a - b);
	print (a * b)
    end;

begin
    test
end.

