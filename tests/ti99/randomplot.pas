program randomplot;

// Test program for the random number generator
// Plotting points with consecutive random numbers allows for an
// easy visual detection of non-random structures.

uses bitmap;

var
    i: integer;

begin
    setVideoMode (BitmapMode);
    setBackColor (black);
    setCellColor (white, black);
    for i := 1 to 32768 do
        plot (random (256), random (192));
    waitKey
end.