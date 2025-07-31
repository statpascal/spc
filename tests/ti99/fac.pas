program factest;

function fac (n: integer): integer;
    begin
        if n = 0 then
            fac := 1
        else
            fac := n * fac (pred (n))
    end;
    
procedure test;
    var 
        i: 1..7;
    begin
        for i := 1 to 7 do
            writeln (i, '!', '=', fac (i))
    end;
    
begin
    test;
    waitkey
end.
