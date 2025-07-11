program withtest;

type
    rec1 = record
	a, b: integer
    end;
    rec2 = record
	r1: rec1;
	c, d: integer
    end;

procedure test1;
    var
        r: rec2;
    begin
        with r, r1 do
            a := 5;
        writeln (r.r1.a)
    end;

procedure test2;
    var
	r: rec2;
    procedure test2internal;
	begin
	    with r do
		with r1 do
		    a := 6
	end;
    begin
   	test2internal;
	writeln (r.r1.a)
    end;

procedure test3 (r: rec2);
    begin
        with r, r1 do
            a := 7;
        writeln (r.r1.a)
    end;

procedure test4 (var r: rec2);
    begin
        with r, r1 do
            a := 8;
    end;

procedure test5;
    var 
	r1: rec1;
 	r2: rec2;
	n: integer;
    begin
	with r1, r2 do begin
	    a := 9;
	    r1.a := 10;
	    c := 11;
	    n := 12
	end;
	writeln (r1.a);
	writeln (r2.r1.a);
	writeln (r2.c);
	writeln (n)
    end;

procedure test6;
    type
	rec2ptr = ^rec2;
    var 
        r2: rec2;

    function f: rec2ptr;
	begin
	    f := addr (r2)
	end;

    begin
	with f^ do 
	    r1.a := 13;
	writeln (r2.r1.a)
    end;

procedure test7;
    var
	r, s: rec1;
	a: integer;
    begin
	with r, s do
	    begin
  	        a := 14;
		r.a := 15
	    end;
	a := 16;
	writeln (s.a);
	writeln (r.a);
	writeln (a)
    end;

var
    r: rec2;	

begin
    test1;
    test2;
    test3 (r);
    test4 (r);
    writeln (r.r1.a);
    test5;
    test6;
    test7
end.
