program ptrtest;

type
    pint = ^integer;
    rec = record
        a: integer;
        p: pint
    end;

var 
    a, b: integer;

function aptr: pint;
    begin
        aptr := addr (a)
    end;

function rectest: rec;
    var 
	res: rec;
    begin
	res.a := 5;
	res.p := addr (a);
        rectest := res
    end;


begin
    aptr^ := 42;
    b := aptr^;
    writeln (b);
    rectest.p^ := 23;
    writeln (a);
    rectest.p := addr (a);
    rectest.a := 4711;
end.
