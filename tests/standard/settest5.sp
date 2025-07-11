program settest1;

var
    s: set of 0..255;
    a, b, c: 0..255;

begin
    a := 5;
    b := 10; c := 15;
    s := [1, 29, 12, 255, a, b..c];
    for a := 0 to 255 do
	if a in s then
	    write (a, ' ');
    writeln
end.
