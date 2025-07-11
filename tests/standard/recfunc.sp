program recfunc;

const
    n = 5;

type
    func1 = function: integer;
    func2 = function (n: integer): integer;
    func3 = function (f: func1): func1;
    proc1 = procedure;
    proc2 = procedure (n: integer);

    rec = record
	f1: func1;
	f2: func2;
 	f3: func3;
	p1: proc1;
	p2: proc2;
        p3: array [1..n] of proc2
    end;
    recptr = ^rec;

var 
    r, s: rec;
    p: recptr;

function function1: integer;
    begin
	function1 := 1
    end;

function function2 (n: integer): integer;
    begin
	function2 := succ (n)
    end;

function function3 (f: func1): func1;
    begin
	function3 := f
    end;

procedure procedure1;
    begin
	writeln ('procedure 1')
    end;

procedure procedure2 (n: integer);
    begin
	writeln ('n = ', n)
    end;

function recfunc: rec;
    begin
	recfunc := r
    end;

function recptrfunc: recptr;
    begin
	recptrfunc := addr (r)
    end;

function copyrecval (r: rec): rec;
    begin
	copyrecval := r
    end;

function copyrecref (var r: rec): rec;
    begin
	copyrecref := r
    end;

procedure setrecord (var r: rec);
    var
	i: 1..n;
    begin
        r.f1 := function1;
        r.f2 := function2;
	r.f3 := function3;
        r.p1 := procedure1;
        r.p2 := procedure2;
	for i := 1 to n do
	    r.p3 [i] := procedure2;
    end;

procedure callref (var r: rec);
    begin
        writeln ('Call Ref');
        writeln (r.f1, ' ', r.f2 (5), ' ', r.f3 (r.f1));
        r.p1; 
        r.p2 (10);
        r.p3 [2] (10)
    end;

procedure callval (r: rec);
    begin
        writeln ('Call Val');
        writeln (r.f1, ' ', r.f2 (5), ' ', r.f3 (r.f1));
        r.p1; 
        r.p2 (10);
        r.p3 [2] (10)
    end;

begin
    setrecord (s);
    r := copyrecval (copyrecref (s));

    writeln ('Record call');
    writeln (r.f1, ' ', r.f2 (5), ' ', r.f3 (r.f1));
    r.p1; 
    r.p2 (10);
    r.p3 [2] (10);

    writeln ('Function result call');
    writeln (recfunc.f1, ' ', recfunc.f2 (6), ' ', recfunc.f3 (recfunc.f1));
    recfunc.p1;
    recfunc.p2 (11);
    recfunc.p3 [3] (11);

    writeln ('Function result pointer call');
    writeln (recptrfunc^.f1, ' ', recptrfunc^.f2 (7), ' ', recptrfunc^.f3 (recptrfunc^.f1));
    recptrfunc^.p1;
    recptrfunc^.p2 (12);
    recptrfunc^.p3 [1] (12);

    callref (r);
    callval (r);
    callval (copyrecval (r));

    new (p);
    p^ := recptrfunc^;
    callval (p^);
    callref (p^);
    dispose (p)
end.
