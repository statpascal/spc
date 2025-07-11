program inlinetest;

var
    a: real;

begin
    a := 1.2;
    writeln (abs (a):10:6);
    a := -1.2;
    writeln (abs (a):10:6);
    a := 0.0;
    writeln (abs (a):10:6);
    a := -0.0;
    writeln (abs (a):10:6);
    a := 25;
    writeln (sqrt (a):10:6);
    a := 2;
    writeln (sqrt (a):10:6)
end.
    