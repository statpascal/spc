program localqual;

var
    n: integer;

procedure outer;
    var
	n: integer;

     procedure inner;
	var
	    n: integer;
	begin
	    localqual.n := 5
        end;

    begin
	inner
    end;

begin
    outer;
    writeln (n)
end.
