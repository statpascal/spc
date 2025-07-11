program test7;

procedure p;
var
    a: vector of vector of int64;
    i: integer;

begin
    for i := 1 to 5 do begin
    a := combine (a, combine (1, 2, 3));
    a := combine (a, combine (4, 5, 6))
    end;
    writeln (size (a));
    for i := 1 to size (a) do
        writeln (a [i])
end;

begin p end.