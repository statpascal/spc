program randtest;

var 
    i: integer;

begin
    for i := 1 to 100 do 
        write (random (5), ' ');
    writeln;
    for i := 1 to 100 do
	write (random:10:8, ' ');
    writeln;
    writeln (randomdata (5, 100));
    writeln (randomdata (100))
end.
