program bittest;

var
    i: uint16;

begin
    for i := 120 * 256 to 140 * 256 do
	writeln (i:5, (i shr 8) xor $ff:5)
end.