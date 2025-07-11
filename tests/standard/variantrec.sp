program variantrec;

type
    rec1 = record
	a, b: integer;
	case c: integer of
	    1: (d, e: real);
	    2: (f, g: integer)
    end;

    rec2 = record
	case boolean of
	    false: (a: int64);
	    true: (p: ^char);
    end;

    rec3 = record
	case char of
	    'A', 'B': (
		case boolean of
		    true: (a: integer);
		    false: (b: real)
		);
	    'C': (
		c: integer;
		case d: integer of
		    1: (e: boolean)
		)
    end;

var
    r1: rec1;
    r2: rec2;
    r3: rec3;
    ch: char;

type
    pchar = ^char;

function ptr (var a): pchar;
    begin
	ptr := addr (a)
    end;

begin
    writeln ('sizeof (rec1) = ', sizeof (rec1));
    writeln ('sizeof (rec2) = ', sizeof (rec2));
    writeln ('sizeof (rec3) = ', sizeof (rec3));

    writeln ('offset r1.a = ', ptr (r1.a) - ptr (r1));
    writeln ('offset r1.b = ', ptr (r1.b) - ptr (r1));
    writeln ('offset r1.c = ', ptr (r1.c) - ptr (r1));
    writeln ('offset r1.d = ', ptr (r1.d) - ptr (r1));
    writeln ('offset r1.e = ', ptr (r1.e) - ptr (r1));
    writeln ('offset r1.f = ', ptr (r1.f) - ptr (r1));
    writeln ('offset r1.g = ', ptr (r1.g) - ptr (r1));

    r2.a := 0;
    r2.p := ptr (ch);
    writeln ('Difference is ', r2.a - int64 (ptr (ch)));

    writeln ('offset r3.a = ', ptr (r3.a) - ptr (r3));    
    writeln ('offset r3.b = ', ptr (r3.b) - ptr (r3));    
    writeln ('offset r3.c = ', ptr (r3.c) - ptr (r3));    
    writeln ('offset r3.d = ', ptr (r3.d) - ptr (r3));    
    writeln ('offset r3.e = ', ptr (r3.e) - ptr (r3))
end.
