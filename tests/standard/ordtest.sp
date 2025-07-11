program ordtest;

type
    enum = (a, b, c, d, e, f, g);

var
    i: enum;

begin
    for i := d downto b do
	writeln (ord (i))
end.