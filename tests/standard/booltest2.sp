program test;

var 
    a, b: boolean;

begin
    writeln ('a':10, 'b':10, 'and':10, 'or':10, 'xor':10, 'not a':10);
    for a := false to true do
	for b := false to true do
	    writeln (a:10, b:10, a and b:10, a or b:10, a xor b:10, not a:10)
end.
