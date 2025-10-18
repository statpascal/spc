program vdptables;

uses vdp;

const
    Tables = 5;
    name: array [1..tables] of string [11] = ('image', 'color', 'pattern', 'sprite attr', 'sprite pat');
    col: array [TVideoMode] of integer = (13, 20, 27);
    mode: array [TVideoMode] of string [4] = ('text', 'std', 'bmp');

var
    table: array [TVideoMode, 1..Tables] of integer;
    i: 1..Tables;
    j: TVideoMode;
    
begin
    for j := BitmapMode downto TextMode do
        begin
            setVideoMode (j);
            table [j, 1] := imageTable;
            table [j, 2] := colorTable;
            table [j, 3] := patternTable;
            table [j, 4] := spriteAttributeTable;
            table [j, 5] := spritePatternTable
        end;

    for j := TextMode to BitmapMode do
        begin
            gotoxy (col [j], 0);
            write (mode [j])
        end;
        
    for i := 1 to Tables do
        begin
            gotoxy (0, succ (i));
            write (name [i]);
            for j := TextMode to BitmapMode do
                begin
                    gotoxy (col [j], succ (i));
                    if table [j, i] = -1 then
                        write ('n/a')
                    else
                        write (hexstr (table [j, i]))
                end
        end;
        
    waitkey
end.
