program euklid;

function ggt (a, b: int64): int64;
    begin 
	if a = 0 then
	    ggt := b
	else
	    begin
  	        while b <> 0 do
		    if a > b then
		        dec (a, b)
		    else
		        dec (b, a);
		ggt := a
	    end;
    end;

procedure test;
    const 
	n = 100;
    var 
	i, j, sum: int64;
    begin
	sum := 0;
	for i := 1 to n do
	    for j := 1 to n do
		inc (sum, ggt (i * 100, j * 100));
	writeln (sum)
    end;

begin
    test
end.


	