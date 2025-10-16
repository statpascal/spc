program lines;

uses bitmap;

type
    TLineData = array [0..1, 0..1] of integer;

const 
    s: TLineData = ((10, 20), (180, 150));
    d: TLineData = ((5, -3), (4, 5));
    
var
    p, q: array [0..1] of TLineData;
    count: integer;
    
procedure drawLine (var p: TLineData);
    begin
        line (p [0, 0], p [0, 1], p [1, 0], p [1, 1])
    end;    
    
procedure add (var p, q: TLineData);
    const
        maxv: array [0..1] of integer = (255, 191);
    var
        i, j: integer;
    begin
        for i := 0 to 1 do
            for j := 0 to 1 do
                begin
                    inc (p [i, j], q [i, j]);
                    if p [i, j] < 0 then
                        begin
                            p [i, j] := abs (p [i, j]);
                            q [i, j] := -q [i, j]
                        end
                    else if p [i, j] > maxv [j] then
                        begin
                            p [i, j] := maxv [j] shl 1 - p [i, j];
                            q [i, j] := - q [i, j]
                        end
                end
    end;    

begin
    p [0] := s;
    p [1] := s;
    q [0] := d;
    q [1] := d;
    
    setVideoMode (BitmapMode);
    
    count := 0;
    repeat
        inc (count);
        setForeColor (TColor (2 + (count div 10) mod 14));
        drawline (p [0]);
        add (p [0], q [0]);
        if count > 50 then
            begin
                setForeColor (transparent);
                drawLine (p [1]);
                add (p [1], q [1])
            end
    until keypressed
end.

