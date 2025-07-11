program qsortvec (output);

const
    n = 1000 * 1000;
    rep = 10;

type 
    element = cardinal;
    vec = vector of element;

function qsort (a: vec): vec;
    var
        median: element;
    begin
        if size (a) <= 1 then
            qsort := a
        else
            begin
                median := a [size (a) div 2];
                qsort := combine (
                    qsort (a [a < median]), 
                    a [a = median],
                    qsort (a [a > median]))
            end
    end;

procedure sim;
    var 
        a, b: vec;
        i: 1..rep;
    begin
        a := randomperm (n);
        for i := 1 to rep do
            b := qsort (a)
    end;

begin
    sim
end.
