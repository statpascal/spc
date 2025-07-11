program test;

type 
    realfunc = function (x: real): real;

var
    i: integer;

function integrate (a, b: real; f: realfunc): real;
    const
	n = 5000;
    var
	i: integer;
	sum, f1, f2, x, w2i: real;
    begin
	sum := 0.0;
	w2i := (b - a) / (2 * (n - 1));
	f1 := f (a);
	for i := 1 to n do
            begin
		x := i / n * b + (n - i) / n * a;
		f2 := f (x);
		sum := sum + (f1 + f2) * w2i;
		f1 := f2
	    end;
	integrate := sum
    end;

function f (x: real): real;
    var 
	res: real;
	j: integer;
    begin
   	res := x;
	for j := 2 to i do
	    res := res * x;
	f := res
    end;

begin
    for i := 1 to 5 do
	writeln (i:2, integrate (0, 1, f):10:5)
end.
