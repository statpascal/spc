program overloadtest;

procedure test (var a: integer);
    begin
	writeln ('var int ', a)
    end;

procedure test (a: integer);
    begin
	writeln ('integer ', a)
    end;

procedure test (a: real);
    begin
	writeln ('real ', a)
    end;

procedure test (a: int64vector);
    begin
        writeln ('intvec ', a)
    end;

procedure test (a: realvector);
    begin
  	writeln ('realvec ', a)
    end;

type 
    proc = procedure (a: real);

var
    p: proc;
    n: integer;

begin
    n := 3;
    test (2);
    test (2.5);
    test (n);
    test (intvec (1, 5));
    test (realvec (1, 2, 0.2));
    p := test;
    p (2.5)
end.
