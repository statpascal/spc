program deeprec;

function f (n: integer): integer;
    begin
        if n = 0 then
            f := 0
        else
            f := 1 + f (n - 1)
    end;

begin
    writeln (f (25 * 1000))
end.