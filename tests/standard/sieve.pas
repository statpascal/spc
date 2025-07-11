const n = 1000;

var count: integer;

procedure doSieve;
    var 
        prim: array [2..n] of boolean;
        r, i, j: integer;

    begin
        for i := 2 to n do prim [i] := true;
        for i := 2 to n do
            if prim [i] then begin
                j := 2 * i;
                while j <= n do begin
                    prim [j] := false;
                    inc (j, i)
                end
            end;
        count := 0;
        for i := 2 to n do
            inc (count, ord (prim [i]))
    end;

begin
    count := 0;
    doSieve;
    writeln (count, ' prime numbers found below ', n)
end.