program cyclicptr;

type
    t1 = ^t2;
    t2 = ^t1;

var
    p: t1; 
    q: t2;

begin
    p := q
end.