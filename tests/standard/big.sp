program big;

const
    n = 5 * 1000 * 1000 * 1000;
    m = 2 * 1000 * 1000 * 1000;

procedure test1;
    var
        a, b, c: int64;
    begin
        a := n;
        inc (a, n);
	writeln (a);
	b := -n;
	writeln (b);
	c := b + n;
	writeln (c);
	c := 10;
        a := c * n;
	writeln (a);
	a := b div n;
	writeln (a);
	a := b mod n;
	writeln (a);
	a := m + m;
	writeln (a);
	c := m;
	c := c + m;
        writeln (c);
    end;

function test2 (p: PChar): int64;
    var
        errorCount: int64;
	q: ^char;
        i: int64;
    begin
	errorCount := 0;
	q := p;
        i := 0;
        while i < n do
	    begin
		q^ := chr (i and 255);
		inc (q, 1000);
		inc (i, 1000)
	    end;
	i := 0;
	while i < n do
	    begin
  	        if ord (p [i]) <> i and 255 then
		    inc (errorCount);
		inc (i, 1000)
	    end;
	test2 := errorCount;
    end;

procedure test3;
    type
	rec = record
	    filler: array [1..n] of char;
	    val: byte;
	end;
    var
	p: ^rec;
    begin
	getmem (p, sizeof (rec));
	p^.val := 17;
	writeln (p^.val);
	writeln (int64 (addr (p^.val)) - int64 (addr (p^.filler)));
	freemem (p, sizeof (rec))
    end;

procedure test4;
    const
	n = 2 * 1000 * 1000 * 1000;
    type
	rec = record
	    filler: array [1..n] of char;
	    subrec: record
		a: int64;
		b: array [1..n] of char
	    end
	end;
    var
	p: ^rec;
    begin
	getmem (p, sizeof (rec));
	p^.subrec.b [n - 10] := 'A';
	freemem (p, sizeof (rec))
    end;

var 
    p: ^char;

begin
    test1;
    getmem (p, n);
    writeln ('test2: ', test2 (p), ' errors');
    freemem (p, n);
    test3;
    test4
end.

