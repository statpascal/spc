program series;

function myexp (x: real): real;
    const
	eps = 1e-10;
    var
	val, sum1, sum0: real;
	k: integer;
    begin
	val := 0;
	sum1 := 1;
	k := 1;
	repeat
	    sum0 := sum1;
	    val := val + sum1;
	    sum1 := sum1 * x / k;
	    inc (k)
	until abs (sum1) < eps;
	myexp := val
    end;

procedure test;
    const 
	n = 10;
    var
	i: -n..n;
    begin
	for i := -n to n do
	    writeln (i / n:7:2, myexp (i / n):12:7)
    end;

begin
    test
end.
	