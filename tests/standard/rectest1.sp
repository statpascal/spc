program rectest;

type
    rec = record
	n: int64;
	c: char
    end;

var
    a, b: rec;

begin
    writeln ('sizeof (rec) = ', sizeof (rec));
    writeln ('sizeof (a) = ', sizeof (a));
    writeln ('addr (b) - addr (a) = ', addr (b) - addr (a))
end.
