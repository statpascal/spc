program seektest;

var
    f: file of integer;
    i, val: integer;

begin
    assign (f, 'DSK1.SEEKTEST');
    rewrite (f);
    for i := 0 to 10 do
        write (f, i);
    close (f);

    reset (f);
    seek (f, 5);
    for i := 5 to 7 do
        begin
            val := i + 10;
            write (f, val)
        end;

    seek (f, 11);
    for i := 11 to 15 do
        write (f, i);

    seek (f, 0);
    while not eof (f) do
        begin
            read (f, i);
            write (i:4)
        end;
    erase (f);
    close (f);

    waitkey
end.
