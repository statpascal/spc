program cfunctest;
uses cfuncs;

procedure systemtest;
    var
	retval: int64;
    begin
        retval := system ('uname');
        writeln ('System call returned: ', retval);
	system ('uname')
    end;

procedure filetest;
    var 
	f: fileptr;
        a, b: int64;
        c, d: word;
    begin
        a := 5;
        c := 7;
	f := fopen ('/tmp/test.bin', 'wb');
        fwrite (addr (a), sizeof (a), 1, f);
        fwrite (addr (c), sizeof (c), 1, f);
        fclose (f);

        f := fopen ('/tmp/test.bin', 'rb');
        fread (addr (b), sizeof (b), 1, f);
        fread (addr (d), sizeof (d), 1, f);
        fclose (f);

	writeln ('b = ', b, ', d = ', d)
    end;

begin
    systemtest;
    filetest
end.
