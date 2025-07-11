program prim_goto;

label 
    no_prime;

const 
    n = 100;

var 
    i, j: integer;

begin
    write (2, ' ');
    i := 3;
    while i <= n do begin
        j := 3;
        while sqr (j) <= i do
            if i mod j = 0 then 
                goto no_prime
            else 
                j := j + 2;
        write (i, ' ');
	no_prime:
        i := i + 2
    end
end.
