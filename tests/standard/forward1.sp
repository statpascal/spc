program forwardtest;

procedure test (a: integer); forward;

type 
    int = integer;

procedure test (a: integer);
    var 
 	b: int;
    begin
	b := a;
	writeln ('b = ', b)
    end;

procedure q (a: integer);

    procedure test (a: integer);
        begin
	    writeln (a)
        end;

    begin
	test (a)
    end;

begin
    test (6);
    q (5)
end.
