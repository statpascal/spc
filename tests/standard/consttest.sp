program consttest;

const
    minval = 1; 
    maxval = 10;

    s1 = 'Hello, world!';
    s2: string = s1;

    set1 = [1, 2, 3, 10];
    set2: set of minval..maxval = set1;

var
    i: minval..maxval;

begin
    writeln ('Hello, world');
    writeln (s1);
    writeln (s2);
    for i := minval to maxval do
	if i in set1 then
	    write (i:3);
    for i := minval to maxval do
	if i in set2 then
	    write (i:3);
    for i := minval to maxval do
	if i in [1, 2, 3, 10] then
	    write (i:3);
    writeln
end.