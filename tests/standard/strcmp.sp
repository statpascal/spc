program simple;

uses math;

type 
    cmpfunc = function (a, b: pointer): integer;

function strcmp (a, b: pointer): integer;
    type
        byteptr = ^byte;
    var
        s: byteptr absolute a;
        t: byteptr absolute b;
        res: integer;
        done: boolean;
    begin
        repeat
            res := sign (s^ - t^);
            done := (res <> 0) or (t^ = 0);
            inc (s); inc (t)
        until done;
        strcmp := res
    end;

var
    i: integer;
    a, b: string;
    cmp: cmpfunc;

begin
    cmp := strcmp;
    a := '123'; b := '456';
    for i := 1 to 10 do
        writeln (cmp (addr (a [1]), addr (b [1])));
end.