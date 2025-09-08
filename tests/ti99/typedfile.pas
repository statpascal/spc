program typedfile;

var
    f: file of integer;
    g: file of string [20];
    i: integer;
    s: string [20];
    
begin
    assign (f, 'DSK1.TEST.DAT');
    rewrite (f);
    assign (g, 'DSK1.TEST1.DAT');
    rewrite (g);
    for i := 1 to 100 do
        begin
            write (f, i);
            str (i:3, s);
            s := 'Line ' + s;
            write (g, s)
        end;
    close (f);
    close (g);
    
    reset (f);
    while not eof (f) do
        begin
            read (f, i);
            write (i)
        end;
    close (f);
    writeln;
    
    reset (g);
    while not eof (g) do
        read (g, s);
    writeln (s);
    close (g);
    
    waitkey    
end.

        