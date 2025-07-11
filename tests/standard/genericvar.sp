program genericvar;

procedure test (var a; kind: integer);
    begin
	case kind of
	    1: writeln (integer (a));
	    2: writeln (real (a):10:8);
	    3: writeln (char (a));
            4: writeln (boolean (a));
	    else
		writeln ('Unknow type')
	end
    end;

var 
    b: integer;
    c: real;
    d: char;
    e: boolean;

begin
    b := 5;
    c := exp (1);
    d := 'A';
    e := true;
    test (b, 1);
    test (c, 2);
    test (d, 3);
    test (e, 4)
end.
