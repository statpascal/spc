program pointertype;

procedure simpletype;
    var
        a: integer;
        b: single absolute a;
        p: pointer;

    begin
        a := 5;
        p := @a;
        single (p^) := 1.23;
        writeln (b:5:3)
    end;

procedure proc1;
    begin
	writeln ('proc1')
    end;

procedure proc2;
    begin
	writeln ('proc2')
    end;

procedure routinetype;
    type
 	proctype = procedure;
    var
        p: pointer;
    begin
	p := @proc1;
        proctype (p);
	p := @proc2;
	proctype (p)
    end;

begin
    simpletype;
    routinetype
end.
