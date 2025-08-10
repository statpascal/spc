program sieve (output);

const 
    n = 10000;
    rep = 1;

var
    prim:  array [0..n] of boolean;
    i, j, k, count: integer;

begin
    writeln ('Starting sieve');
    for k := 1 to rep do begin
        for i := 0 to n do
            prim [i] := true;
        i := 2;
        while i * i <= n do
            begin
                if prim [i] then
                    begin
                        j := i + i;
                        while j <= n do
                            begin
                                prim [j] := false;
                                inc (j, i)
                            end
                    end;
                inc (i)
            end;
    end;
    count := 0;
    for i := 2 to n do
        if prim [i] then
            begin
                write (i:8);
                inc (count)
            end;
    writeln; writeln;
    writeln ('Found ', count, ' primes below ', n);
    waitkey
end.

