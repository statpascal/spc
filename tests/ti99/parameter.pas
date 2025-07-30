program parameter;

const
    n = 10;

type
    field = array [1..n] of integer;
    
procedure fillit1 (var a: field);
    var
        i: integer;
    begin
        for i := 1 to 10 do
            a [i] := succ (n) - i
    end;
    
function fillit2: field;
    var
        a: field;
        i: integer;
    begin
        for i := 1 to 10 do
            a [i] := i;
        fillit2 := a
    end;
    
procedure printit (a: field);
    var
        i: integer;
    begin
        for i := 1 to 10 do
            begin
                writeint (a [i]);
                writechar (' ')
            end
    end;
    
function addit (a, b: field): field;
    var
        i: integer;
        res: field;
    begin
        for i := 1 to 10 do
            res [i] := a [i] + b [i];
        addit := res
    end;
        
var
    a, b: field;
    
begin
    fillit1 (a);
    printit (a);
    writelf;
    a := fillit2;
    b := a;
    printit (b);
    writelf;
    printit (addit (fillit2, fillit2));
    waitkey
end.
