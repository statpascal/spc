program settest;

procedure simpletest;
    const
        maxset = 255;

    var 
        s: set of 10..25;
        t: set of 20..40;
        u: set of 10..30;
        v, w: set of char;

    type
        intset = set of 0..maxset;
        charset = set of char;

    procedure printintset (s: intset);
        var 
            i: 0..maxset;
        begin
            for i := 0 to maxset do
                if i in s then
                    write (i, ' ');
            writeln;
	    for i := 0 to maxset do
	        if not (i in s) then
		    write (i, ' ')
        end;

    procedure printcharset (s: charset);
        var
            ch: char;
        begin
            for ch := chr (0) to chr (255) do
                if ch in s then
                    write (ch, ' ');
	    writeln
        end;

    procedure compareset (s, t: intset);
	begin
	    write ('[');
	    printintset (s);
	    write ('] op [');
	    printintset (t);
	    write (']: ');
(*	    
	    Less than/greater than for sets is non-standard:
	    writeln (s = t, ' ', s <> t, ' ', s <= t, ' ', s < t, ' ', s >= t, ' ', s > t)
*)
	    writeln (s = t, ' ', s <> t, ' ', s <= t, ' ', (s <= t) and (s <> t), ' ', s >= t, ' ', (s >= t) and (s <> t))
	end;

    begin
        w := ['A', 'B', 'F'..'H'];
        v := w;
        v := [];
        s := [10, 11, 20..25];
        t := [22..27, 30, 35];

        u := s + t;
        printintset (u);
        writeln;
        printintset (t - s);
        writeln;
        printintset (s - t);
        writeln;

        printcharset (w);
	printcharset ([]);
	printcharset (['A']);
	printcharset (['A'..'Z']);
	printcharset (['a'..'z', '1'..'9']);
	printcharset (['0'..'9', 'A', 'D', 'F']);

	w := (w - ['A'..'F']) * (v - w) + ['A'..'G'] * ['B'..'H'] + (['X', 'Y'] + v) * ['X'];
        printcharset (w);

	compareset ([1, 2, 3], [1, 2, 3]);
	compareset ([1, 2], [1, 2, 3]);
	compareset ([1, 2, 3], [1, 2]);
	compareset ([1, 2, 3], [1, 2, 4]);
	compareset ([], [1, 2, 3]);
	compareset ([1, 2, 3], []);
	compareset ([], []);
	writeln (w = [], ' ', [] = w, ' ', [] = [], ' ', [] <= [], ' ', [] <= w, ' ', w <= [])
    end;

procedure enumtest;
    type
	enum = (v0, v1, v2, v3, v4, v5);
	enumset = set of enum;
    var 
	s: set of v0..v3;
	t: set of v1..v5;

    procedure printset (s: enumset);
	var 
	    i: enum;
	begin
	    for i := v0 to v5 do
		if i in s then
		    write ('v', ord (i), ' ');
	    writeln	
	end;

    begin
	s := [v0, v1];
        t := [v1..v3, v5];
        printset (s + t);
	printset (s * t);
	printset (s - t);
	printset (t - s);
	writeln (v3 in s, ' ', v0 in s)
    end;

procedure setbincount;
    const
	maxbits = 4;
    type
	bitrange = 0..maxbits;
	bitset = set of bitrange;
    var
	bits: bitset;
	bit: bitrange;
        n: integer;

    function pow2 (n: integer): integer;
	var
	    res, i: integer;
	begin
	    res := 1;
	    for i := 1 to n do
		res := res * 2;
	    pow2 := res
	end;

    procedure printset (n: integer; bits: bitset);
	var 
	    i: bitrange;
	begin
	    write (n:3, ' ');
	    for i := maxbits - 1 downto 0 do
		write (ord (i in bits));
	    writeln
	end;

    begin
        bits := [];
        for n := 0 to pow2 (maxbits) - 1 do 
	    begin
		printset (n, bits);
		bit := 0;
		while bit in bits do
		    begin
			bits := bits - [bit];
			bit := succ (bit)
		    end;
		bits := bits + [bit]
	    end
    end;

procedure setconsttest;
    type
        intset = set of 0..255;

    const
        s1 = [];
        s2 = [2, 3, 5, 7, 11, 13, 17, 19];
        s3: intset = [];
        s4: intset = s2;
        s5: intset = [1..5, 10, 12];

    var
        s: intset;
        ch: char;

    procedure print (s: intset);
        var
            i: integer;
        begin
            for i := 0 to 255 do
                if i in s then
                    write (i, ' ');
            writeln
        end;

    function isAlpha (ch: char): boolean;
        const
            alpha = ['A'..'Z', 'a'..'z'];
        begin
            isAlpha := ch in alpha
        end;

    function isNumber (ch: char): boolean;
	const
	    number: set of char = ['0'..'9'];
	begin
	    isNumber := ch in number
	end;

    begin
        s := s1;
        print (s);
        s := s2;
        print (s);
	writeln (s1 = s2, ' ', s1 <> s2, ' ', s1 <= s2, ' ', s2 <= s1);

        print (s1);
        print (s2);
        print (s3);
        print (s4);
        print (s5);
        s3 := s2 + s5;
        print (s3);

        for ch := chr (0) to chr (255) do
            if isAlpha (ch) then
                write (ch);
        writeln;
        for ch := chr (0) to chr (255) do
            if isNumber (ch) then
                write (ch);
        writeln
    end;
		
begin
    simpletest;
    enumtest;
    setconsttest;
    setbincount
end.
