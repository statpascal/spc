program readme;

var
    s: string;

begin
    close (input);
    assign (input, 'DSK1.README');
    reset (input);

    if IOResult <> 0 then
        begin
            writeln ('Cannot open DSK1.README');
            writeln ('Expected: DIS/VAR254');
            waitkey;
            exit
        end;

    while not eof do
        begin
            readln (s);
            writeln (s)
        end;
    close (input);

    writeln;
    writeln ('Now copying readme.md to TEXT.TXT');

    close (output);
    assign (output, 'DSK1.TESTTXT');
    reset (input);
    rewrite (output);

    while not eof do
        begin
            readln (s);
            writeln (s)
        end;

    close (input);
    close (output);

    assign (output, '');
    rewrite (output);
    writeln ('Copy done');

    waitkey
end.
