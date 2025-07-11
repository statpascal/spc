program qsortvec;

type 
    element = int32;
    vec = vector of element;

function qsort (a: vec): vec;
    begin
        if size (a) <= 1 then
            qsort := a
        else
           qsort := combine (
               qsort (a [a < a [1]]), 
               a [a = a [1]], 
               qsort (a [a > a [1]]))
    end;

const
    n = 20;
var 
    a: vec;

begin
    a := randomperm (n);
    writeln ('Unsorted: ', a);
    writeln ('Sorted:   ', qsort (a))
end.
