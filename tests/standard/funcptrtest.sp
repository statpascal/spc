program funcptrtest;

type
    cmpfunc = function (a, b: string): integer;

var
    f: cmpfunc;

function strcmp (a, b: string): integer;
    begin
	strcmp := ord (b >= a)
    end;

procedure setcmpfunc (g: cmpfunc);
    begin
	f := g
    end;

begin
    setcmpfunc (strcmp);
    writeln (f ('a', 'b'));
    writeln (f ('b', 'a'))
end.