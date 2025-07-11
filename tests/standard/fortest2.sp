program fortest2;

type 
    integer = int64;

const
    a = 1;
    b = 10;

var
    i, sum: integer;

begin
    sum := 0;
    for i := -a to b - 1 do
	inc (sum, i);
    writeln (sum)
end.
