program pattern;

uses vdp;

var
    pat: TCharPattern;
    ch: 'A'..'F';
    i: integer;
    
procedure showCross (x, y: integer);
    const
        size = 5;
    var
        i: integer;
    begin
        for i := 0 to size do
            begin
                showHChar (x, y + i, 130, size + 1);
                gotoxy (x + i, y + i);
                write (chr (128));
                gotoxy (x + size - i, y + i);
                write (chr (129))
            end
        end;

begin
    setVideoMode (StandardMode);
    setBackColor (black);
    setTextColor (lightgreen);
    for ch := 'A' to 'F' do
        writeln (ch, ': ', getCharPattern (ord (ch)));
        
    setCharPattern (128, '8040201008040201');	// '\'
    setCharPattern (129, '0102040810204080');   // '/'
    setCharPattern (130, '');
    setColor (128 div 8, darkblue, lightyellow);

    for i := 0 to 3 do
        begin
            showCross (8 * i + 1, 8);
            showCross (8 * i + 1, 15)
        end;
        
    waitkey
end.
