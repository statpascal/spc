program charset;

var
    ch: char;
    s: set of char;

begin
    s := ['A'..'F', 'X', 'Y'];
    for ch := chr (0) to chr (255) do
	if ch in s then
	    write (ch:2)
end.


