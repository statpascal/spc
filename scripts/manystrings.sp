program manystrings;

var 
    i, j: integer;

begin
    writeln ('program manystrings;');
    writeln;
    for i := 1 to 100 do
        begin
            writeln ('procedure p', i, ';');
            writeln ('begin');
            for j := 1 to 50 do
                writeln ('    writeln (''This is line '', ', 50 * pred (i) + j, ');');
	    writeln ('end;');
	    writeln
       end;

    writeln ('begin');
    for i := 1 to 100 do
        writeln ('    p', i, ';');
    writeln ('end.')
end.

