program exittest;

function f (n: integer): integer;
    begin
	if n = 0 then begin
	    f := 5;
	    exit
	end;
	f := 1
    end;

procedure p (n: integer);
    begin
	if n = 0 then begin
	    writeln ('n = 0');
	    exit
	end;
	writeln ('n <> 0')
    end;

begin
    writeln (f (0):5, f (1):5);
    p (0); 
    p (1)
end.
