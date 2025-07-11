program funcret;

type
    number = int64;
    func = function: number;

function f: number;
    begin
	f := 5
    end;

function g: func;
    begin
	g := f
    end;

begin
    writeln (f, ' ', g)
end.
