program float;

type
    singlefunc = function: single;

var 
    x: double;
    y: single;
    h: singlefunc;

function f (x: single): single;
    begin
	f := cos (x)
    end;

function g: single;
    begin
	g := f (0.6)
    end;

function g1: singlefunc;
    begin
        g1 := g
    end;

procedure print (x: single);
    begin
	writeln (x:5:3)
    end;

begin
    x := 1.5;
    y := x;
    x := y;
    print (y);
    print (x);
    print (f (0.5));
    print (g);
    print (g + y);
    h := g;
    print (h);
    print (g1)
end.
