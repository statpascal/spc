program blockrw;

var
    i: integer;
    f: file;
    buf1, buf2: array [0..255] of byte;

begin
    for i := 0 to 255 do
        buf1 [i] := i; 

    assign (f, 'DSK1.TEST.DAT');
    rewrite (f, 128);
    blockwrite (f, buf1, 2);
    close (f);
    
    reset (f, 128);
    blockread (f, buf2, 2);
    close (f);
    
    for i := 0 to 255 do 
        write (hexstr2 (buf2 [i]));
    writeln;        
   
    writeln ('VDP FREE: ', hexstr (memW [$8370 div 2]));
    waitkey
end.