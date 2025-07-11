program stringpara;

procedure test (a: string);
    begin
	writeln (a);
	a := 'test-changed';
	writeln (a)
    end;

procedure test1;
    var
	a: string;
    begin
	a := 'test1';
	writeln (a);
	test (a);
	writeln (a)
    end;

begin
    test1
end.
