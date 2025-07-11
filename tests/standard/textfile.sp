program filetest;

procedure testEmptyFile;
    var
	f: text;
    begin
	assign (f, '/tmp/test.txt');
	rewrite (f);
	close (f);

	reset (f);
   	writeln (eoln (f), ' ', eof (f));
	close (f);
	erase (f)
    end;

procedure testWriteRead;
    const 
        n = 10;
    var
	ch: char;
        i: 1..n;
        n1, n2, n3, n4: integer;
        x1, x2: real;
        s: string;
	charCount: integer;
        f: text;

    procedure testReadChar (var f: text);
        var
            ch: char;
        begin
            reset (f);
            while not eof (f) do
                begin
                    read (f, ch);
                    write (ch)
                end;
            writeln;
            close (f)
        end;

    procedure testReadReal (var f: text);
        var 
            a: real;
        begin
            reset (f);
            while not eof (f) do
                begin
                    read (f, a);
                    write (a:10:5)
                end;
            writeln;
            close (f)
        end;

    begin
        assign (f, '/tmp/test.txt');
        rewrite (f);
        for i := 1 to n div 2 do
            writeln (f, 'This is line ', i);
        close (f);
	append (f);
        for i := n div 2 + 1 to n do
            writeln (f, 'This is line ', i);
	close (f);

        reset (f);
        while not eof (f) do
            begin
                readln (f, s);
		writeln (s)
            end;
	close (f);

	reset (f);
	while not eof (f) do 
	    begin
                charCount := 0;
		while not eoln (f) do 
                    begin
			read (f, ch);
                        write (ch);
			charCount := succ (charCount)
		    end;
		readln (f);
		writeln (':  ', charCount, ' characters')
	    end;
	close (f);

        rewrite (f);
        writeln (f, 1, ' ', -2, '   ', 0, ' ', 5);
        writeln (f, 3.1415926:10:8, exp (1):15:8);
        close (f);

        reset (f);
        readln (f, n1, n2, n3);
        readln (f, x1, x2);
        close (f);
        writeln (n1:5, n2:5, n3:5);
        writeln (x1:10:8, x2:15:8);

	reset (f);
	read (f, n1, n2, n3, n4);
	read (f, s);
        if s = '' then
	    writeln ('s empty')
	else
  	    writeln ('ERROR: s should be empty but is: "', s, '"');
	close (f);

	testReadReal (f);
	testReadChar (f);

	rewrite (f);
        writeln (f, 1, ' ', -2, '   ', 0, ' ', 5);
        write (f, 3.1415926:10:8, exp (1):15:8);
	close (f);

	testReadChar (f);
	testReadReal (f);
	erase (f)
    end;       

begin
    testEmptyFile;
    testWriteRead
end.
