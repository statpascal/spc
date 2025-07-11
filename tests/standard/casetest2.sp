program case1;

var 
    i: uint16;

begin
    for i := 1 to 200 do
	case i of
	    5:
		writeln (5);
	    10..20:
		writeln (i, ' 10..20');
	    21..30:
		writeln (i, ' 21..30');
	    35:
		writeln (35)
	end
end.
