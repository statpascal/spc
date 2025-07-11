program fortest;

procedure bytetest;
    var
        i: byte;

    begin
        for i := 5 downto 0 do
            writeln (i);
        writeln (i);
        for i := 250 to 255 do
            writeln (i);
        writeln (i)
    end;

procedure enumtest;
    type
        enum = (a, b, c, d, e, f, g);
        subenum = b..e;
    var
        i: enum;
        j: subenum;
	k: (value);
    begin
        for i := a to g do 
            writeln (ord (i));
        for i := g downto a do
	    writeln (ord (i));
	for j := b to e do
	    writeln (ord (j));
	for j := e downto b do
	    writeln (ord (j));
 	for j := e to b do 
	    writeln ('Error');
	for k := value to value do
	    writeln (ord (k));
	for k := value downto value do
	    writeln (ord (k))
    end;

procedure booltest;
    var
        b: boolean;
        c: true..true;
    begin
	for b := false to true do
	    writeln (b);
	for b := true downto false do
	    writeln (b);
	for c := true downto true do
	    writeln (c)
    end;

begin
    bytetest;
    enumtest;
    booltest
end.
