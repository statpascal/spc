program vectest;

type
    intv = vector of integer;
    rec = record
	a: integer;
	b: vector of real
    end;

var
    a, b, d: intv;
    c: vector of boolean;
    e: vector of byte;
    r: rec;

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
    b := intvec (2, 3);
    c := a > 2;
    writeln (size (a));
    writeln (a);
    writeln (a [b]);
    writeln (c);
    writeln (a [c]);
    c := a <= 3;
    d := combine (a, b);
    print (d);
    writeln (a [3]);
    print (a);
    print (add (a));
    r.b := a;
    writeln (r.b);
    e := intvec (5, 10);
    writeln (e)
end.
