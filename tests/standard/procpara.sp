program procpara;

procedure p1;
    begin
	writeln ('p1')
    end;

procedure p2 (n: integer);
    begin
	writeln ('p2: ', n)
    end;

type
    proc1 = procedure;
    proc2 = procedure (n: integer);

procedure test1 (p: proc1; q: proc2);
    begin
	p;
	q (5)
    end;

function f1: integer;
    begin
	f1 := 6
    end;

function f2 (n: integer): integer;
    begin
	f2 := n + 1
    end;

type
    func1 = function: integer;
    func2 = function (n: integer): integer;

procedure test2 (f: func1; g: func2);
    begin
 	writeln (f);
	writeln (g (6))
    end;

begin
    test1 (p1, p2);
    test2 (f1, f2)
end.
