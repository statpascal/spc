program incdec;

procedure inttest;
    var
        a: integer;
    begin
        a := 5;
        inc (a);
	writeln ('a = ', a);
	dec (a);
	writeln ('a = ', a);
        inc (a, 2);
	writeln ('a = ', a);
	dec (a, 3);
	writeln ('a = ', a)
    end;

procedure enumtest;
    type
	enum = (v1, v2, v3, v4);
    var
	a: enum;
    begin
	a := v1;
	inc (a);
	write (ord (a):3);
	inc (a, 2);
	write (ord (a):3);
	dec (a);
 	write (ord (a):3);
	dec (a, 2);
	writeln (ord (a):3)
    end;

procedure ptrtest;
    const
	n = 5;
    var
	a: array [1..n] of integer;
        p: ^integer;
	i: 1..n;
    begin
	for i := 1 to n do
	    a [i] := 21 + i;
	p := addr (a [1]);
	for i := 1 to n do
	    begin
		write (p^:3);
		inc (p)
	    end;
	writeln;
	inc (p, -1);
	while p >= addr (a [1]) do
	    begin
		write (p^:3);
		dec (p, 2)
	    end;
	writeln
    end;

procedure opttest;

    var
	a, b, c: integer;

    procedure incdec (var a: integer; f: boolean);
 	var
	    d: integer;
	begin
	    d := 9;
	    if f then
		begin
		    inc (a);
		    inc (b);
		    inc (d)
		end
	    else
		begin
		    dec (a);
		    dec (b);
		    dec (d)
		end;
	    c := d;
	end;

    begin
	a := 5;
	b := 7;
	incdec (a, true);
	writeln (a, ' ', b, ' ', c);
	incdec (b, false);
	writeln (a, ' ', b, ' ', c)
    end;

begin
    inttest;
    enumtest;
    ptrtest;
    opttest;
end.

    