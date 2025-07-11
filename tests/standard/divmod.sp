program divmod;

const
    n = 15;

var 
    i: -n..n;
    j: 0..n;

begin
    for i := -n to n do
        writeln ('Block 1: ', i:4, i div 2:4, i mod 4:4, i div 4:4, i mod 5:4, i div 5:4);
    for j := 0 to n do
        writeln ('Block 2: ', j:4, j div 2:4, j mod 4:4, j div 4:4, j mod 5:4, j div 5:4);
    for i := -n to n do
        writeln ('Block 3: ', i:4, i div (-2):4, i mod (-4):4, i div (-4):4, i mod (-5):4, i div (-5):4);
    for j := 0 to n do
        writeln ('Block 4: ', j:4, j div (-2):4, j mod (-4):4, j div (-4):4, j mod (-5):4, j div (-5):4)
end.