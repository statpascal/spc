program casetest3;

var
    ain, a1, b1, c1, d, d1: integer;

procedure test (var a: integer; b: integer);
    var
	c: integer;
    begin
	c := b;
	case a of
	    5: 
		inc (a);
	    6: 
		a1 := a
	end;
	case b of
	    5:
		b1 := b - 1;
	    6:
		b1 := b + 2
	end;
	case c of
	    9: 
		c1 := c - 1
	end;
	case d of
	    11:
		inc (d);
	    12:
		d1 := d
	end 
    end;

begin
    a1 := 0; b1 := 0; c1 := 0; d1 := 0;
    ain := 5; d := 11;
    test (ain, 5);
    writeln (ain:5, a1:5, b1:5, c1:5, d:5, d1:5);
    test (ain, 6);
    writeln (ain:5, a1:5, b1:5, c1:5, d:5, d1:5);
    test (ain, 9);
    writeln (ain:5, a1:5, b1:5, c1:5, d:5, d1:5);
    test (ain, 15);
    writeln (ain:5, a1:5, b1:5, c1:5, d:5, d1:5)
end.