program rangecheck;

type  
    subrange = 1..10;

procedure routinetest (n: subrange);
    begin
        writeln (n)
    end;

procedure test;
    var 
        a: -10..30;
        b: integer;
        d: word;
        c: int64;
        f: array [1..5] of -7..-3;

    procedure looptest;
        begin
            for a := 1 to 10 do
                routinetest (a);
            f [3] := -5;
        end;

    function func: subrange;
        begin
            func := 3
        end;

    procedure assigntest;
        begin
            a := func;
            a := 15;
            b := a;
            d := a;
            a := b;
            c := a;
            c := b
        end;

    begin
        looptest;
	assigntest
    end;

begin
    test
end.
