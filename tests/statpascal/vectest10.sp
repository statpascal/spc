program vectest;

type
    intv = vector of integer;

var
    a, b: intv;

procedure print (a: intv);
    begin
	writeln (a)
    end;

function add (a: intv): intv;
    begin
        add := a + 1
    end;

begin
    a := intvec (1, 5);
    b := add (a);
    print (b);
end.
