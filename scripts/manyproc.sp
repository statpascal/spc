program manyproc;

const
    n = 10 * 1000;

var 
    i: 1..n;

begin
    writeln ('program bigproc;');
    writeln ('var i: integer;');
    for i := 1 to n do
        begin
            writeln ('procedure p', i, ';');
            writeln ('begin');
            writeln ('    i := i + 1');
	    writeln ('end;');
        end;
    writeln ('begin');
    writeln ('    i := 0;');
    for i := 1 to n do
        writeln ('    p', i, ';');
    writeln ('    writeln (i)');
    writeln ('end.')
end.
