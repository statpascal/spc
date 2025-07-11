program vec;

var
    x: vector of vector of int64;
    i: integer;

begin
    x := combine (1, 2, 3);
    x := combine (x, combine (4, 5, 6));
    for i := 1 to size (x) do
        writeln (x [i]);
end.