program testtest3;

type
    rec = record
	a: integer;
	s: string
    end;
    recarray = array [1..5] of rec;

var
    r: recarray;

procedure p (a: char; r: recarray);
    begin
	writeln (a, r [3].s);
    end;

begin
    r [3].s := 'ello';
    p ('H', r);
end.
