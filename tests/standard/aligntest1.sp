program aligntest;

type 
    field = array [1..10] of char;

var
    b: field;

procedure p1;
    begin
        writeln ('p1')
    end;

procedure p2 (a, b, c, d, e: char);
    begin
	writeln ('p2 ', a, b, c, d, e)
    end;

procedure p3;
    var 
	a: char;
    begin
        a := 'A';
        writeln ('p3 ', a)
    end;

procedure p4 (a: integer);
    begin
	writeln ('p4 ', a)
    end;

procedure p5;
    var
	a: field;
    begin
	a [1] := 'A';
        a [10] := 'B';
	writeln ('p5 ', a [1], a [10])
    end;

procedure p6 (a: field);
    begin
	writeln ('p6 ', a [1], a [10])
    end;

procedure p7 (var a: field);
    begin
	writeln ('p7 ', a [1], a [10])
    end;

procedure p8 (b: field);
    var
	a: field;
    begin
	a [1] := 'A';
	a [10] := 'B';
	writeln ('p8 ', a [1], a [10], b [1], b [10])
    end;


begin
    p1;
    p2 ('A', 'B', 'C', 'D', 'E');
    p3;
    p4 (65000);
    p5;
    b [1] := 'C';
    b [10] := 'D';
    p6 (b);
    p7 (b);
    p8 (b)
end.
