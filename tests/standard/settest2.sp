program test;

var
    s, t, u: set of 0..255;
    i, n: integer;

begin
    n := 8;
    s := [7, 10, 15, 33, 40..50, n];
    t := [1];
    for i := 0 to 255 do
        if i in s + t then 
	    write (i, ' ');
    writeln
end.
