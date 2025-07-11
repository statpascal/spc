program c_call;

function pow (x, y: real): real; external;
procedure printf (var p: char; a, b: integer); external;

var 
    fmt: string;

begin
    writeln (pow (2, 3):3:0, ' ', pow (2, 0.5):8:6);
    fmt := 'The values are %d and %d' + chr (10) + chr (10);
    printf (fmt [1], 2, 3)
end.
