program incdec;

const 
    n = 10;

function concat (a, b: string): string;
    begin
	concat := a + b
    end;

procedure test;
    var
        s, t: string;
	count: int64;
        i: 1..n;
    begin
 	count := 0;
	for i := 1 to n do 
	    begin
		str (i, s);
		t := 'Hello, ' + s;
		inc (count, length (t))
	    end;
	writeln (count)
    end;


begin
    test
end.
