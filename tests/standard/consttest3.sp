program consttest3;

type
    enum = (v1, v2, v3, v4);

procedure p1 (a: integer);
    begin
	writeln ('p1, a = ', a)
    end;

procedure p2 (a: real);
    begin
    end;

const
    a: int64 = 5;
    b: int32 = 6;
    c: int16 = 8;
    d: int8 = 9;
    e: char = 'A';
    f: enum = v2;
    s: string = 'String 1';
    arr: array [5..10] of int32 = (1, 2, 3, 4, 5, 6);
    rec: record a: int32; b: char; s: string end = (a: 17; b: 'C'; s: 'String 2');
    setv: set of uint8 = [5, 9, 17, 61, 139];
    g: procedure (a: integer) = p1;
    x: real = 3.1415;
    y: single = 2.71828;

begin
    writeln (x:8:4, ' ', y:8:4);
    g (5)
end.


