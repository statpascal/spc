program pointertest;

type
    a = integer;

procedure outer;
    var
	a: real;

    procedure inner;
	type
	    p = ^a;
	var
	    q: p;
	begin
	    writeln (sizeof (q^))
	end;

    begin
	inner
    end;

begin
    outer
end.
    