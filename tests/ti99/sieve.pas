program sieve (output);

const 
    n = 10000;
    rep = 1;

var
    prim:  array [0..n] of boolean;
    i, j, k: integer;

begin
    writeint (-100); writechar (' ');
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
    for i := 2 to n do
        if prim [i] then
            begin
  	        writeint (i);
	        writechar (' ')
	    end;
    waitkey
end.

