program mulopt;

const 
    count = 2;

var
    a, b, c, x, y, z: real;
    i, m, n, o, p: int64;

function add (a, b: real): real;
    begin
	add := a + b
    end;

begin
    for i := -count to count do
        begin
	    a := 2.3; b := 7.5; c := -9.3;
	    o := 4; p := 5;
	    n := 4 + (p * 5 + o * (p + o - (3 * i div 2)));
	    m := n - (i - (-o) * i + o * (p - n * (i mod 5 + 1)) * (o - n * i and 5)) or 1;
	    x := a / b * (i mod 10 + 0.1);
	    y := (i mod 2) / c + 0.3;
	    z := add (a + x * y, b * (x) * y + a * (x + y - a * (b + i) * x - (x / y + a * b)) * b);
	end;
    writeln (z:8:3, ' ', m);
end.

