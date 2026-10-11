program sieve (output);

const 
    n = 1000;
    rep = 1;
    showPrimes = true;

var
    prim:  array [0..n] of boolean;
    i, j, k, count: integer;

begin
    writeln ('Starting sieve');
    for k := 1 to rep do 
        begin
            for i := 0 to n do
                prim [i] := true;
            i := 2;
            while sqr (i) <= n do
                begin
                    if prim [i] then
                        begin
                            j := i + i;
                            while j <= n do
                                begin
                                    prim [j] := false;
                                    j := j + i
                                end
                        end;
                    i := i + 1
                end
        end;
    count := 0;
    for i := 2 to n do
        if prim [i] then
            begin
                count := count + 1;
                if showPrimes then
                    begin
                        write (i:5);
                        if count mod 7 = 0 then
                            writeln
                    end
            end;
    writeln; writeln;
    writeln ('Found ', count, ' primes below ', n);
    waitkey
end.

