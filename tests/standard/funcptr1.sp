program funcptr1;

type
    func = function: integer;
    funcret = function: func;

function f1: integer;
    begin
	f1 := 5
    end;

function f2: func;
    begin
	f2 := f1
    end;

procedure print (a: integer; f: func; g: funcret);
    var 
 	b, c: integer;
    begin
	b := f;
        c := g;
	writeln (a:5, b:5, c:5)
    end;

begin
    print (f2, f2, f2)
end.
