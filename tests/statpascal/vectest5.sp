program test6;

function q (a: int64vector): int64vector;
    begin
	q := combine (a + 1, a + 2, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109);
        writeln (size (a))
    end;

procedure p;
    var a: vector of int64;
begin
    a := 17;
    writeln (q (combine (a, a, a)))
end;

begin p end.