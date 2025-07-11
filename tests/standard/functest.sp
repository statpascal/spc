program functest;

function f1: integer;
    function f2: integer;
        begin
	    f2 := 2
 	end;
    begin
	f1 := f2 + (f2 * f2)
    end;

begin
    writeln (f1:f1)
end.
