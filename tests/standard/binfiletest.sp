program binfiletest;

procedure typedfiletest;
    const 
	n = 100;
    var
	f: file of integer;
	i, pos: 0..n;
        j: 1..n * n;
	val: integer;
    begin
	assign (f, '/tmp/typed.bin');
	rewrite (f);
	for i := 0 to pred (n) do
	    write (f, i);
	writeln (filesize (f), ' records in file, pos is ', filepos (f));
 	close (f);

	reset (f);
	for j := 1 to n * n do
	    begin
		pos := random (n);
		seek (f, pos);
		val := 0;
		read (f, val);
		if val <> pos then
		    writeln ('Error in reading typed file (expected ', pos, ' but got ', val);
		val := filepos (f);
		if val <> pos + 1 then
		    writeln ('Error in filepos (expected ', pos, ' but got ', val)
	    end;
	close (f);
  	erase (f)
    end;

procedure untypedfiletest;
    var
        f: file;
        m, n: int64;
        k: word;

    begin
        assign (f, '/tmp/untyped.bin');
        rewrite (f, 1);

        n := $0102030405060708;
        blockwrite (f, n, sizeof (n));
	writeln (filesize (f), ' records in file, pos is ', filepos (f));
	seek (f, 3);
	blockread (f, k, sizeof (k));
	writeln ('k = ', k, ', pos is ', filepos (f));
        close (f);

        reset (f, sizeof (m));
	writeln (filesize (f), ' records in file');
        blockread (f, m, 1);
        close (f);
	erase (f);
        writeln ('m = ', m);
    end;

begin
    typedfiletest;
    untypedfiletest
end.
