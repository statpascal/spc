program stackframe;

var
    glob_i: integer;
    glob_s: string [20];

procedure p1;
    var
	level1_i: integer;
	level1_s: string [20];

    procedure p2;
        var
            level2_i: integer;
            level2_s: string [20];

        procedure p3;
            var
                level3_i: integer;
                level3_s: string [20];

	    begin
		glob_i := 0;
		level1_i := 1;
		level2_i := 2;
		level3_i := 3;
		glob_s := 'S0';
		level1_s := 'S1';
		level2_s := 'S2';
		level3_s := 'S3';
		writeln ('p3: ', glob_i, level1_i, level2_i, level3_i, glob_s, level1_s, level2_s, level3_s)
	    end;

	begin
	    p3;
            writeln ('p2: ', glob_i, level1_i, level2_i, glob_s, level1_s, level2_s)
	end;

    begin
	p2;
        writeln ('p1: ', glob_i, level1_i, glob_s, level1_s)
    end;

begin
    p1;
    writeln ('main: ', glob_i, glob_s);
    waitkey
end.
