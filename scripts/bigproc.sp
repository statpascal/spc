program bigproc;

const
    n = 1000 * 1000;

var 
    i: 1..n;

begin
    writeln ('program bigproc;');
    writeln;
    writeln ('procedure big;');
    writeln ('    var i: integer;');
    writeln ('    begin');
    writeln ('        i := 0;');
    for i := 1 to n do
        writeln ('        i := i + 1;');
    writeln ('    writeln (i)');
    writeln ('end;');
    writeln;
    writeln ('begin');
    writeln ('    big');
    writeln ('end.')
end.
