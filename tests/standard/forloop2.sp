program fortest2;

(* Test aliases for loop variable *)

var
    i: integer;

procedure test1;

    procedure test2 (var a: integer);
	begin
	    for i := 1 to 5 do
		write (a:5);
	    writeln
	end;

    begin
	test2 (i)
    end;

procedure test2;
    var
	j: integer;
        p: ^integer;
    begin
	p := addr (j);
	for j := 1 to 5 do
	    write (p^:5);
  	writeln
    end;

begin
    test1;
    test2
end.

    

