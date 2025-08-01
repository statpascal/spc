program sieve (output);

const 
    n = 1000;
    size = n div 8 + 1;
    rep = 1;

var
    prim:  array [0..size] of uint8;
    i, j, k, count: integer;

begin
    writeln ('Starting sieve');
    for k := 1 to rep do begin
        for i := 0 to size do
            prim [i] := $ff;
        i := 2;
        while i * i <= n do
            begin
                if prim [i shr 3] and (1 shl (i and 7)) <> 0 then
                    begin
                        j := i + i;
                        while j <= n do
                            begin
                                prim [j shr 3] := prim [j shr 3] and not (1 shl (j and 7));
                                inc (j, i)
                            end
                    end;
                inc (i)
            end;
    end;
    count := 0;
    for i := 2 to n do
        if prim [i shr 3] and (1 shl (i and 7)) <> 0 then
            begin
                write (i:4);
                inc (count)
            end;
    writeln; writeln;
    writeln ('Found ', count, ' primes below ', n);
    waitkey
end.

