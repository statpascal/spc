program vectest3;

type
    enum = (a, b, c, d, e, f, g, h);

var
    v: vector of enum;
    i: enum;
    j: integer;

begin
    for i := a to h do
	v := combine (v, i);
    for j := 1 to size (v) do
	write (ord (v [j]):2);
    writeln
end.

