program test;

var
    s: string;
    b: char;
    i: integer;

begin
    s := 'ABCDEF';
    b := s [2];
    writeln (b);
    s [3] := 'c';
    writeln (s);
    for i := 1 to length (s) do
	write (ord (s [i]): 5);
    writeln;
    i := 1;
    while (i <= length (s)) and (s [i] <= 'C') do
	inc (i);
    while i <= length (s) do 
	begin
	    write (s [i]);
	    inc (i)
	end;
    writeln
end.
