program booltest;

function f (n: integer): boolean;
    begin
	f := odd (n)
    end;

var 
    i: integer;

begin
    for i := (-2) * (-5)  downto 10 div (-1) do
	writeln (i:5, ord (f (i)):7)
end.
