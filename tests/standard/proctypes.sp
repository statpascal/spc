program proctypes;

type 
    proc = procedure;
    procint = procedure (a: integer);

procedure proc1;
    begin
        writeln (5)
    end;

procedure proc2 (a: integer);
    begin
        writeln (a)
    end;

procedure procvarpara1 (var f: proc);
    begin
        f := proc1
    end;

procedure procvarpara2 (var f: procint);
    begin
        f := proc2
    end;

procedure procvalpara1 (f: proc);
    begin
        f
    end;

procedure procvalpara2 (f: procint);
    begin 
        f (6)
    end;

function procres1: proc;
    begin
        procres1 := proc1
    end;

function procres2: procint;
    begin
        procres2 := proc2
    end;

    
var
    o, p, q: proc;
    r, s, t: procint;

begin
    p := proc1;
    q := p;
    q;
    r := proc2;
    s := r;
    s (6);
    procvarpara1 (o);
    o;
    procvarpara2 (t);
    t (6);
    procvalpara1 (proc1);
    procvalpara2 (proc2);
    procvalpara1 (p);
    procvalpara2 (r);
    q := procres1;
    q;
    s := procres2;
    s (6)
end.

