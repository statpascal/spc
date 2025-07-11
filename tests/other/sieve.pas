const n = 1000;

var count: integer;

procedure DoSieve;

    var prim: array [2..n] of boolean;
        r, i, j: integer;

    begin
        for i := 2 to n do prim [i] := true;
        for i := 2 to n do
            if prim [i] then begin
                j := 2 * i;
                while j < n do begin
                    prim [j] := false;
                    j := j + i
                end
            end;
        count := 0;
        for i := 2 to n do
            count := count + ord (prim [i])
    end;

procedure Sim;
    var k: integer;
    begin
        for k := 1 to 100000 do
            DoSieve
    end;

begin
    count := 0;
    Sim;
    writeln (count, ' prime numbers found below ', n)
end.