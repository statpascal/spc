program test5;

procedure p;
    var
	a: vector of int64;
	b: vector of int8;
        c: vector of boolean;
        v: int64;
    begin
	a := intvec (1, 20);
	b := intvec (5, 10);
	a [5] := 10;
	writeln (a);
	writeln (a [b]);
	c := a < 10;
        writeln (c);
        writeln (a [c])
    end;

begin
    p
end.

   