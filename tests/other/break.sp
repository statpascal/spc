program breakpoint;

procedure callback (a: integer; b: real); export;
    begin
	writeln ('Hello from callback: ', a, ' ', b)
    end;

begin
    writeln ('Before');
    break;
    writeln ('After breakpoints')
end.
