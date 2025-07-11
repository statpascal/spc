program initconst;

type
    rec = record
        n: integer;
	x: real;
	s: string
    end;
    field1 = array [1..5] of integer;
    field2 = array [1..2] of rec;
    enum = (val1, val2, val3, val4);

const
    a: integer = 5;
    b: real = 3.1415;
    c: string = 'Hello, world';
    d: boolean = true;
    e: field1 = (1, 2, 3, 4, 5);
    f: field2 = ((n: 1; x: 2.1; s: 'ABC'), (n: 2; x: 2.2; s: 'DEF'));
    g: enum = val3;

begin
    writeln (f [2].s)
end.

