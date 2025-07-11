program exprtest;

function f (n: integer): integer;
    begin
	f := succ (n)
    end;

var 
    a: integer;

begin
    a := (f) (5);
    writeln (a)
end.

