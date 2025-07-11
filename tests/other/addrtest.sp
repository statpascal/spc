program addrtest;

type 
    rec = record
	a, b: integer
    end;

var
    r: rec;
    p: ^rec;

begin
    p := addr (r);
    p^.a := 5;
    writeln (r.a)
end.
