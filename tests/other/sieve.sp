program sieve;

const 
    n = 10 * 1000 * 1000;

var 
    prim: array [1..n] of boolean;
    i: 1..n;
    j: 6..2 * n;

begin
    prim [1] := false;
    prim [2] := true;
    for i := 3 to n do
        prim [i] := odd (i);

    i := 3;
    while sqr (i) <= n do 
        begin
            if prim [i] then 
                begin
                    j := 2 * i;
                    while j <= n do  
                        begin
                            prim [j] := false;
                            j := j + i
                        end
                end;
            i := i + 2
        end;
    for i := 1 to n do
        if prim [i] then
            write (i, ' ')
end.
