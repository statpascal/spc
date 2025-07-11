program test;

type
    stringarray = array [1..5] of string;
    rec1 = record
	a: char;
	b: real
    end;
    rec1array = array [1..5] of rec1;
    rec2 = record
        a: integer;
        s: string
    end;
    rec2array = array [1..5] of rec2;
    rec3 = record
	a, b: char
    end;
    rec3array = array [1..5] of rec3;
    rec4 = record
	a, b: word
    end;
    rec4array = array [1..5] of rec4;

procedure align1 (a: char; s: stringarray);
    begin
        writeln (a, s [1], s [5])
    end;

procedure align2 (a: byte; s: rec1array);
    begin
        writeln (a, ' ', s [5].b:8:5)
    end;

procedure align3 (a: char; s: rec2array);
    begin
        writeln (a, s [4].s)
    end;

procedure align4 (a: char; s: rec3array);
    begin
        writeln (a, s [3].b)
    end;

procedure align5 (a: char; s: rec4array);
    begin
        writeln (a, s [2].b)
    end;

var
    s1: stringarray;
    s2: rec1array;
    s3: rec2array;
    s4: rec3array;
    s5: rec4array;

begin
    s1 [1] := 'S1';
    s1 [5] := 'S5';
    align1 ('A', s1);

    s2 [5].b := 3.473;
    align2 (200, s2);

    s3 [4].s := 'S4';
    align3 ('B', s3);

    s4 [3].b := 'F';
    align4 ('E', s4);

    s5 [2].b := 60000;
    align5 ('F', s5)
end.
