program casttest;

procedure assignment;
    var 
	a: integer;
	b: byte absolute a;
    begin
	a := 257;
        b := 0;
        writeln ('a = ' , a)
    end;

procedure value;
    var
	a: integer;
	b: byte;
    begin
	a := 257;
	b := byte (a);
 	writeln ('b = ', b, ' ', byte (a), ' ', byte (257))
    end;

begin
    assignment;
    value
end.

