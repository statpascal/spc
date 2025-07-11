program dummy;

type
    stringvec = vector of string;

procedure print (var a: stringvec);
    var
        i: integer;
    begin
        for i := 1 to size (a) do
            writeln (a [i]);
        writeln
    end;

procedure fill (var a: stringvec; start, stop: integer);
    var
        i: integer;
	s: string;
    begin
        for i := start to stop do
	    begin
		str (i, s);
                a [i] := 'Hello, world #' + s
	    end
    end;        

procedure test;
    var
        a: stringvec;
    begin
        resize (a, 10);
        fill (a, 1, size (a));
        print (a);

        resize (a, 5);
        print (a);

        resize (a, 15);
        fill (a, 6, size (a));
        print (a);
    end;

begin
    test
end.
