program sizeoftest;

const
    n = 100;

type
    rec1 = record
        a, b: integer;
        f: boolean
    end;
    field1 = array [1..sizeof (rec1)] of byte;
    field2 = array [1..n] of rec1;
    field3 = array [1..(n div 2), 1..(2 * n)] of field1;
    field4 = array [1..3] of char;
    rec2 = record
	a: field4;
        b: char;
        c: field4;
        d: boolean;
        e: field4;
        f: word
    end;
    field5 = array [1..2] of rec2;
    field6 = array [1..2] of 
        record
            a: char;
            b: word
        end;

var
    a1: rec1;
    a2: field1;
    a3: field2;
    a4: field3;
    a5: field4;
    a6: rec2;
    a7: field5;
    a8: field6; 

procedure testnested;
    type 
	a = integer;
    procedure nested;
        var 
	    a: array [1..10] of integer;
	begin
	    writeln ('nested: ', sizeof (a))
	end;

    begin
	nested
    end;

begin
    writeln ('sizeof (rec1) = ', sizeof (rec1));
    writeln ('sizeof (field1) = ', sizeof (field1));
    writeln ('sizeof (field2) = ', sizeof (field2));
    writeln ('sizeof (field3) = ', sizeof (field3));
    writeln ('sizeof (field4) = ', sizeof (field4));
    writeln ('sizeof (rec2) = ', sizeof (rec2));
    writeln ('sizeof (field5) = ', sizeof (field5));
    writeln ('sizeof (field6) = ', sizeof (field6));

    writeln ('sizeof (a1) = ', sizeof (a1));
    writeln ('sizeof (a2) = ', sizeof (a2));
    writeln ('sizeof (a3) = ', sizeof (a3));
    writeln ('sizeof (a4) = ', sizeof (a4));
    writeln ('sizeof (a5) = ', sizeof (a5));
    writeln ('sizeof (a6) = ', sizeof (a6));
    writeln ('sizeof (a7) = ', sizeof (a7));
    writeln ('sizeof (a8) = ', sizeof (a8));

    writeln ('sizeof (a8 [1]) = ', sizeof (a8 [1]));
    writeln ('sizeof (a8 [1].b) = ', sizeof (a8 [1].a));

    writeln ('sizeof (true) = ', sizeof (true));
    writeln ('sizeof (sqr (1000000)) = ', sizeof (sqr (1000000)));

    testnested
end.
