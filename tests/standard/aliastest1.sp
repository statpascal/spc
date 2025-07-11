program test;

procedure p1 (var a);
    var
	b: ^integer;
    begin
	b := addr (a);
	b^ := 5
    end;

procedure p2 (var a);
    var
	b: ^real;
    begin
	b := addr (a);
	b^ := 1.234
    end;

var 
    c: integer;
    d: real;

begin
    p1 (c);
    writeln (c);
    p2 (d);
    writeln (d:10:5)
end.

