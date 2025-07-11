program overflow;

var a, b, c: int64;
    v1, v2: vector of int64;

begin
    a := $7ffffffffffffffe;
    b := 1;
    writeln ('a = ', a, ', b = ', b);
    c := a + b;
    writeln ('c = ', c);
    c := -a - 2;
    writeln ('c = ', c);

    v1 := combine (a, a);
    v2 := combine (0, 1);
    writeln (v1 + v2);
    writeln (v1 + v2 + 0);

    writeln (0-v1 - v2);
    writeln (0-v1 - v2 - 1);

    v1 := combine (2, 3);
    while true do 
        begin
	    writeln (v1);
            v1 := v1 * v1
        end;

    a := 2;
    while true do
        begin
	    writeln ('a = ', a);
            a := a * 2
	end
end.
