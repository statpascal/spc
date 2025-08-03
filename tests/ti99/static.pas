program static;

var
    b: integer;
const
    a: string [20] = 'XYZ';
var 
    e: integer;

procedure p;
    var
	b: integer;
    const
        a: integer = 5;
        s: string [5] = 'ABCDE';
        d: char = 'X';
	e: char = 'Y';
    var	
	c: integer;
    begin
        writeln (a, ' ', s, ' ', d, e)
    end;
    
begin
    p;
    writeln (a);
    writeln (true, ' ', false);
    waitkey;
end.
