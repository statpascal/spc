program retval1;

function test1: real;
    begin
	test1 := 7
    end;

function test2: real;
    var 
	x: single;
    begin
	x := exp (1);
	test2 := x
    end;

function test3: int64;
    var
	a: byte;
    begin
	a := 8;
	test3 := a
    end;

function test4: byte;
    var
	a: integer;
    begin
	a := 9;
	test4 := a
    end;

begin
    writeln (test1:10:5, test2:10:5, test3:5, test4:5)
end.