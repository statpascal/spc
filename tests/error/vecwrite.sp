program dummy;

var
    i: integer;
    a: vector of integer;
    b: vector of char;
    c: vector of string;
    d: vector of boolean;
    e: vector of real;

begin
    a := intvec (1, 10);
    b := 'A';
    for i := ord ('B') to ord ('Z') do
	b := combine (b, chr (i));
    c := combine ('vector', 'of', 'string', 'values');
    d := a <= 5;
    e := realvec (0.0, 1.0, 0.1);

    writeln (a);
    writeln (b);
    writeln (c);
    writeln (d);
    writeln (e);

    writeln ('With formatting:');
    writeln (a:5);
    writeln (b:0);
    writeln (c:20);
    writeln (d:10);
    writeln (e:5:2)
end.
