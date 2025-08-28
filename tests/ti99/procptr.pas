program procptr;

type
    functype = function (a, b: integer): integer;
    proctype = procedure (a, b: integer);
    
function func (a, b: integer): integer;
    begin
        func := a + b
    end;
    
procedure proc1 (a, b: integer);
    begin
        writeln (a:5, b:5)
    end;
    
procedure proc2 (a, b: integer);
    begin
        proc1 (b, a)
    end;
    
procedure para (p1: proctype);
    begin
        p1 (-1, 1)
    end;
    
var
    f: functype;
    p: proctype;
    i: 1..2;
    
const
    pstatic: array [1..2] of proctype = (proc1, proc2);
    
begin
    f := func;
    p := proc1;
    
    p (f (5, 2), f (8, -4));
    
    para (p);
    para (proc1);
    para (proc2);
    
    for i := 1 to 2 do
        pstatic [i] (-2, 2);
    
    waitkey
end.
