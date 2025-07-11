program vectest2;

const
    n = 10;

var
    v: vector of string;
    s: string;
    i: 1..n;

begin
    for i := 1 to n do
	begin
	    str (i, s);
	    v := combine (v, 'This is string ' + s)
	end;
    for i := 1 to size (v) do
	writeln (v [i])
end.

