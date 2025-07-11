program forloop1;

var
    i, j, sum: integer;

function test1: integer;
    var
	i, j, sum: integer;
    begin
	sum := 0;
	for i := 1 to 10 do
	    begin
		for j := i to 10 do
		    inc (sum, j);
		for j := 1 to i do
		    inc (sum, j);
		for j:= 2 + i to 10 - i do
		    inc (sum, j)
	    end;
	test1 := sum
    end;

function test2: integer;
    begin
	sum := 0;
	for i := 1 to 10 do
	    begin
		for j := i to 10 do
		    inc (sum, j);
		for j := 1 to i do
		    inc (sum, j);
		for j := 2 + i to 10 - i do
		    inc (sum, j);
	    end;
	test2 := sum
    end;

function test3: integer;
    var
	i, j, sum: integer;
    begin
	sum := 0;
	for i := -1 downto -10 do
	    begin
		for j := i downto -10 do
		    inc (sum, j);
		for j := -1 downto i do
		    inc (sum, j);
		for j := 2 + i downto 10 - i do
		    inc (sum, j)
	    end;
	test3 := sum
    end;

function test4: integer;
    begin
	sum := 0;
	for i := -1 downto -10 do
	    begin
		for j := i downto -10 do
		    inc (sum, j);
		for j := -1 downto i do
		    inc (sum, j);
		for j := 2 + i downto 10 - i do
		    inc (sum, j)
	    end;
	test4 := sum
    end;

function test5 (a, b: integer): integer;
    var
	i, j, sum: integer;
    begin
	sum := 0;
	for i := a downto b do
	    begin
		for j := i downto b do
		    inc (sum, j);
		for j := a downto i do
		    inc (sum, j);
		for j := 2 + i downto 10 - i do
		    inc (sum, j)
	    end;
	test5 := sum
    end;

function test6 (a, b: integer): integer;
    var
	i, j, sum: integer;
    begin
	sum := 0;
	for i := a to b do
	    begin
		for j := i to b do
		    inc (sum, j);
		for j := a to i do
		    inc (sum, j);
		for j := 2 + i to 10 - i do
		    inc (sum, j)
	    end;
	test6 := sum
    end;

begin
    writeln (test1:5, ' ', test2:5);
    writeln (test3:5, ' ', test4:5);
    writeln (test5 (50, 30));
    writeln (test5 (15, -10));
    writeln (test5 (-10, -20));
    writeln (test5 (-10, 10));
    writeln (test6 (30, 50));
    writeln (test6 (-10, 15));
    writeln (test6 (-20, 10));
    writeln (test6 (10, -10))
end.
