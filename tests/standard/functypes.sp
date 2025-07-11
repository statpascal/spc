program functypes;

type
    func = function: integer;
    funcint = function (a: integer): integer;

function func1: integer;
    begin
        func1 := 6
    end;

function func2 (n: integer): integer;
    begin
        func2 := n + 1
    end;

var 
    f1, g1: func;
    f2, g2: funcint;

begin
    f1 := func1;
    g1 := f1;
    writeln (g1);
    f2 := func2;
    g2 := f2;
    writeln (g2 (6))
end.

