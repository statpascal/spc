program forwardtest;

type
    proc = procedure;
    procint = procedure (a: integer);

var
    p: proc;

procedure proc1; forward;

procedure proc2;
    begin
        p := proc1
    end;

procedure proc1;
    begin
        writeln ('proc1')
    end;

procedure proc3 (b: integer);
    var
        p: procint;

    procedure proc4;
        begin
            p := proc3
        end;

    begin
        if b = 0 then
            begin
                proc4;
                p (1)
            end
        else
            writeln (b)
    end;
    
begin
    proc2;
    p;
    proc3 (0)
end.
