program Simpson;
const
    Pi = 3.14159;
    Pi2 = Pi/2;
type
    RealFunction = function (x: real): real;
var
    Fkt: RealFunction;
    i:  integer;

function Integral (f: RealFunction; a, b: real; n: integer): real;
    var
        h, s:   real;
        i:      integer;
    begin
        n := 2 * n;
        h := (b - a) / n;
        s := f(a) + f(b);
        for i := 1 to n-1 do
            if odd (i) then s := s + 4 * f(a+i*h)
            else s := s + 2 * f (a + i*h);
        Integral := h/3*s;
    end;

function f(x: real): real;
    begin
        f := exp(-sqr(x)/2)
    end;

begin
    WriteLn(Integral(f, 0, 1, 60):10:5)
end.
