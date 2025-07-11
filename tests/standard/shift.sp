program shift;

var
    a: integer;
    b: cardinal;

begin
    a := 314;
    while a <> 0 do 
	begin
	    write (a, ' ');
	    a := a shr 1
	end;
    writeln;

    b := 1;
    while b <> 0 do
        begin
	    write (b, ' ');
	    b := cardinal (b shl 1)
	end;
    writeln;
end.

