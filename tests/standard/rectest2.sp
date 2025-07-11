program rectest2;

type
    rec = record
	a: integer;
	b: real
    end;

var
    c, d: rec;

procedure copy (a: rec; var b: rec);
    begin
	b := a
    end;

begin
    c.a := 2;
    c.b := 1.57;
    copy (c, d);
    writeln (d.a, d.b:5:2)
end.