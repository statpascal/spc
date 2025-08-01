program test;

type
    rec = record
        a, b: integer;
        c, d: char;
        e: string [10];
    end;
    
function makeit: rec;
    var
        r: rec;
    begin
        with r do begin
            a := 5;
            b := 9;
            c := 'A';
            d := 'F';
            e := 'Hello'
        end;
        makeit := r
    end;
    
procedure printit (r: rec);
    begin
        with r do
            write (a:3, b:3, c:3, d:3, ' ', e)
    end;
    
begin
    printit (makeit);
    waitkey
end.
