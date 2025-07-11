program prim0;

const n = 1000 * 1000;

var i, j: integer;
    prim: boolean;

begin
    write (2, ' ');
    i := 3;
    while i <= n do begin
        j := 3;
        prim := true;
        while prim and (sqr (j) <= i) do
            if i mod j = 0 then 
                prim := false
            else 
                j := j + 2;
        if prim then
            write (i, ' ');
        i := i + 2
    end;
    writeln
end.
