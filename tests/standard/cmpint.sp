program dummy;

var
    i: integer;
    x: real;

begin
    for i := -50 to 50 do
	begin
	    x := i;
  	    writeln (i:5, i < 0:10, i <= 0:10, i = 0:10, i <> 0:10, i > 0:10, i >= 0: 10);
  	    writeln (x:5:1, x < 0:10, x <= 0:10, x = 0:10, x <> 0:10, x > 0:10, x >= 0: 10)
	end
end.
