program floatvec;

type
    fvec = vector of single;
    fmat = vector of fvec;

var
    x: vector of single;
    y: vector of real;
    a: single;
    b: vector of byte;
    m: fmat;


procedure print (v: fvec);
    begin
	writeln (v)
    end;

procedure printm (m: fmat);
    var 
	i: integer;
    begin
	for i := 1 to size (m) do
	    writeln (m [i])
    end;

begin
    a := 0;
    x := combine (a, 1.0, 2.0, 3.0);
    writeln (x);
    b := intvec (0, 3);
    x := b;
    writeln (x);
    x := combine (0.0, 1.0, 2.0, 3.0);
    writeln (x);
    y := x;
    writeln (y);
    x := y;
    print (x);
    print (x + x);
    print (x + y);
    m := x;
    m := combine (m, 2 * x, 3 * x);
    writeln ('Matrix:');
    printm (m);

end.
