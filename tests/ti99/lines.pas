program bmptest;

uses bitmap;

var i: integer;

begin
    setColor (lightyellow);
    setBitmapMode;	// fills color table with selected color
    for i := 0 to 191 do
        plot (10 + i, i);
    waitkey
end.

