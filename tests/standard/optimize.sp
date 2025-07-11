program test;

var
    r1, r2, r3: int64;

procedure p1;
    var
	a, b, c: int64;
    begin
	a := 5;
	b := a;
	c := b + b;
	r1 := c;
	c := 3 * a;
	r2 := c;
	c := 4 * a;
	r3 := c
    end;

begin
    p1;
    writeln (r1, ' ', r2, ' ', r3)
end.