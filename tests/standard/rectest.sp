program rectest;

type 
    rec = record
	a: integer;
        s: string
    end;

function testf1: rec;
    var
        res: rec;
    begin
        res.a := 1;
        res.s := 'String';
        testf1 := res
    end;

function testf2: rec;
    var
        res: rec;
    begin
        res.a := 2;
        res.s := 'String';
        testf2 := res
    end;

procedure test1;
    var
        r: rec;
    begin
        r := testf1;
        writeln (r.a, ' ', r.s)
    end;

procedure test2;
    procedure test2_1 (r: rec);
        begin
            writeln (r.a, ' ', r.s)
        end;
    procedure test2_2 (var r: rec);
        begin
            writeln (r.a, ' ', r.s)
        end;
    var
        r: rec;
    begin
        r := testf1;
        test2_1 (r);
        test2_2 (r)
    end;

procedure test3;
    procedure test3_1 (r: rec);
        begin
            writeln (r.a, ' ', r.s)
        end;
    begin
        test3_1 (testf1)
    end;

procedure test4;
    procedure test4_1 (r1, r2: rec);
        begin
	    writeln (r1.a, ' ', r1.s);
            writeln (r2.a, ' ', r2.s)
	end;
    begin
	test4_1 (testf1, testf2)
    end;


begin
    test1;
    test2;
    test3;
    test4
end.