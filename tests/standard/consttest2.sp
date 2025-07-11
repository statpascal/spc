program proctest;

procedure p1;
    begin
	writeln ('P1')
    end;

procedure p2;
    begin
	writeln ('P2')
    end;

const
    n = 4;
    p: array [1..n] of procedure = (p1, p2, p1, p2);

var 
    i: 1..n;

begin
    for i := 1 to n do
        p [i]
end.
