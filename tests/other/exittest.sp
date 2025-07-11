program exittest;

procedure exit;
    begin
    end;

function f (n: integer): integer;
    begin
        if odd (n) then
   	    begin
		f := 1;
		exit
	    end;
	f := 0
    end;

function g (n: integer): integer;
    begin
        if odd (n) then
   	    begin
		g := 1;
		exit
	    end;
	g := 0
    end;

function h (n: integer): integer;
    label
        99;
    begin
        if odd (n) then
   	    begin
		h := 1;
		goto 99
	    end;
	h := 0;
    99:
    end;



begin
    writeln (f (0): 2, f (1): 2, f (2): 2);
    writeln (g (0): 2, g (1): 2, g (2): 2);
    writeln (h (0): 2, h (1): 2, h (2): 2)
end.
