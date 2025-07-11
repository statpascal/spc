program labeltest (output);

label 
    c1, c2, c3, done;
var
    i: 1..3;

procedure count1;
    label
	loop;
    var
	i: integer;
    begin
	i := 1;
        loop:
  	    writeln (i);
	    i := i + 1;
	    if i < 6 then 
		goto loop
    end;

procedure count2;
    label
	done;
    var 
	i: integer;
    begin
	i := 1;
	repeat
 	    writeln (i);
	    i := i + 1;
	    if i > 5 then
	        goto done
	until false;
	done:
    end;

procedure count3;
    label
	1, 2;
    var
	i: integer;
    begin
	i := 1;
	1:
	    writeln (i);
	    i := i + 1;
	    if i > 5 then
	 	goto 2
	    else
		goto 1;
	2:
    end;
	
begin 
    for i := 1 to 3 do
        begin
	    if i = 1 then
		goto c1;
	    if i = 2 then 
		goto c2;
	    if i = 3 then
		goto c3;

	    c1:
		count1;
		goto done;
	    c2:
		count2;
		goto done;
	    c3:
		count3;
		goto done;

	    done:
		writeln
	end
end.
