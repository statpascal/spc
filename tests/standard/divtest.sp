program divtest;

var
    g: real;

procedure test1;
    const
	n = 10;
    var
	i: 1..n;
	a, b, c: int64;
    begin
	a := 5;
	b := 3;
	for i := 1 to n do
	    c := a div b;
        g := c
    end;

procedure test2;
    const
	n = 10;
    var
	i: 1..n;
	a, b, c: real;
    begin
	a := 5;
	b := 3;
	for i := 1 to n do
	    c := a / b;
	g := c
    end;

begin
    test1; writeln (g:5:3);
    test2; writeln (g:5:3)
end.
