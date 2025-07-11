unit math;

interface

function ifthen (f: boolean; a, b: int64): int64;


implementation

function ifthen (f: boolean; a, b: int64): int64;
    begin
        if f then
            ifthen := a
        else
            ifthen := b
    end;

end.
