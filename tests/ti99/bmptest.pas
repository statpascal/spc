program bmptest;

uses bitmap;

var
    i: integer;

begin
    setColor (white);
    setBkColor (black);
    setBitmapMode;	// fills color table with selected color
    
    for i := 1 to 20 do
        line (0, 9 * i, 255, (21 - i) * 9)
end.

