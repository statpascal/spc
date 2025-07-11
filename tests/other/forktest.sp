program forktest;
uses cfuncs;

procedure child;
    var 
	i: integer;
    begin
	for i := 1 to 1000 do
	    write ('Child ', i, ' ')
    end;

procedure parent;
    var 
	i: integer;
    begin
	for i := 1 to 1000 do
	    write ('Parent ', i, ' ')
    end;

procedure run;
    var
        child_pid: pid_t;
	wstatus: integer;
    begin
 	child_pid := fork;
	if child_pid = 0 then
	    parent
	else
	    child;
 	waitpid (child_pid, wstatus, 0)
    end;

begin
    run
end.

