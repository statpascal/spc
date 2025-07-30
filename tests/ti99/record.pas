program test;

type
    rec = record
        a, b: integer;
        c, d: char
    end;
    
function makeit: rec;
    var
        r: rec;
    begin
        with r do begin
            a := 5;
            b := 9;
            c := 'A';
            d := 'F'
        end;
        makeit := r
    end;
    
procedure printit (r: rec);
    begin
        writeint (r.a); writeint (r.b);
        writechar (r.c); writechar (r.d)
    end;
    
begin
    printit (makeit);
    waitkey
end.
