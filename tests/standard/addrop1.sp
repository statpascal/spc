program addrop;

type
    arr = array [1..2] of int64;
    parr = ^arr;

var 
    a: arr;
    p: parr;
    q: ^int64;
    n: integer;

procedure proc (var a: integer);
    begin
	writeln (@a = @n);
	writeln (addr (a) = addr (n))
    end;

begin
    p := @a;
    q := @(p^[2]);
    writeln (int64 (q) - int64 (p) = sizeof (int64));
    writeln (@a = addr (a));
    writeln (@p = addr (p));
    writeln (@p^[1] = addr (p^[1]));
    proc (n);
end.