program regressiontest;

procedure simpletest;
    var 
        a, b, c: byte;
    begin
        a := 20;
        b := 22;
        c := a + b * 3;
        if a = 20 then 
            write (a, ' ')
        else
            write (0, ' ');
        write (c, ' ');
        b := 1;
        while b <= 5 do begin
            write (b, ' ');
            b := b + 1
        end;
        b := 1;
        repeat
            write (b, ' ');
            b := b + 1
        until b > 5
    end;

procedure doubletest;
    var 
        a, b, c, pi: real;
    begin
        a := 20.5;
        b := 21.5;
        c := a + b;
        writeln (c:5:1);
        pi := 3.1415926;
        writeln (pi:10:8)
    end;

procedure fortest;
    var 
        i, j: integer;
    begin
        for i := 1 to 10 do
            for j := 10 downto i do
                write (i, ' ', j, ' - ');
        for i := 10 to 1 do
            write ('nothing');
        for i := 1 downto 10 do
            write ('nothing');
        writeln
    end;

procedure unarytest;
    var 
        a: integer;
        b: boolean;
    begin
        a := 5;
        writeln ('a = ', a, ', -a = ', -a, ', not a = ', not a, ', not not a = ', not not a);
        b := true;
        writeln ('b = ', b, ', not b = ', not b, ', not not b = ', not not b)
    end;

procedure recordtest;
    type 
        rec0 = record 
        end;

        rec1 = record
            a, b: integer;
            c: real;
            d: boolean;
            e: 1..32;
            f: array [1..3] of byte
        end;

    var 
        r1, r2: rec1;

    begin
        r1.a := 1;
        r1.b := 2;
        r1.c := 3.1415926;
        r1.d := true;
        r1.f [2] := 3;
        writeln (r1.b, ' ', r1.f [2]);

        r2 := r1;
        writeln (r2.b, ' ', r2.f [2])
    end;

procedure stringtest;
    type 
        rec = record
            a: integer;
            b: boolean;
            s: string
        end;

        rec1 = record
            x, y: real;
            r: rec
        end;

    var 
        a, b, c: string;
        d, h: rec;
        e: array [1..1000] of string;
        f, g: array [1..10] of rec;
        i: integer;
        p, q, r: rec1;

    procedure testvar (var a: string);
        begin
            a := 'Output parameter'
        end;

    procedure test (a: string);
        var 
            s: string;
            field: array [1..5] of string;
        begin
            writeln (a)
        end;

    begin
        a := 'Hello';
        b := 'world!';
        c := a + ', ' + b;
        d.s := c;
        writeln (d.s);
        for i := 1 to 1000 do 
            e [i] := 'Hallo, world';
        writeln (' ', e [500]);
        test (d.s);

        h := d;
        test (h.s);
     
        for i := 1 to 10 do
            f [i] := h;
        g := f;
        test (f [5].s);

        p.r := f [4];
        q.r := p.r;
        r := q;
        test (r.r.s)
    end;

procedure stringchartest;
    var
        c: char;
        s, t: string;

    begin
        s := 'H';
        s := s + 'ell' + 'o';
        c := 'w';
        t := ' ' + (c + 'or' + 'ld');
        c := ',';
        s := s + c + t;
        writeln (s)
    end;

procedure arraytest;
    var 
        a: array [1..10] of integer;
        i: 1..10;
        j: 1..5;
        b, c: array [1..5, 1..5] of 1..25;
        d: array ['A'..'D'] of 1000000..2000000;
        e: 'A'..'D';
    begin
        for i := 1 to 10 do
            a [i] := i * i;
        for i := 1 to 10 do
            write (a [i], ' ');
        writeln;

        for i := 1 to 5 do
            for j := 1 to 5 do
                b [i, j] := i * j;
        for i := 1 to 5 do
            for j := 1 to 5 do
                write (b [i, j], ' ');
        writeln;

        c := b;
        for i := 1 to 5 do
            for j := 1 to 5 do
                write (c [i, j], ' ');
        
        for e := 'A' to 'D' do
            d [e] := 1500000 + ord (e);
        writeln ('d [''B''] = ', d ['B']);

        writeln
    end;

procedure quicksorttest;
    const 
        n = 20;
    type 
        element = real;
        index = 1..n;
        field = array [index] of element;

    procedure qsort (var a: field; left, right: index);
        var 
            m: element;
            i, j: 0..n + 1;

        procedure swap (var a, b: element);
            var 
                c: element;
            begin
                c := a; a := b; b := c
            end;

        begin
            m := a [(left + right) div 2];
            i := left; j := right;
            repeat
                while a [i] < m do 
                    i := i + 1;
                while a [j] > m do 
                    j := j - 1;
                if i <= j then 
                    begin 
                        swap (a [i], a [j]);
                        i := i + 1; 
                        j := j - 1
                    end
            until i > j;
            if i < right then 
                qsort (a, i, right);
            if left < j then 
                qsort (a, left, j)
        end;

    procedure test;
        var 
	    a: field;
            i: index;
        begin
            for i := 1 to n do a [i] := n + 1 -i;
            qsort (a, 1, n);
            for i := 1 to n do write (a [i]:5:0);
            writeln
        end;
            
    begin
        test
    end;

procedure refpartest;
    var 
        c: real;
        d: integer;

    procedure test1 (var a: real; var b: integer);
    begin
        a := 1.5;
        b := 2
    end;

    procedure test (var a: real; var b: integer);
    begin
        test1 (a, b);
        write (a:5:2, ' ', b)
    end;

    begin
        test (c, d);
        writeln (c:5:2, ' ', d)
    end;

procedure factest;
    const 
        n = 10;
    var 
        i: integer;

    function fac (n: byte): longint;
    begin
        if n > 0 then 
            fac := n * fac (n - 1)
        else
            fac := 1
    end;

    begin
        for i := 1 to n do
            writeln (i, '  ', fac (i))
    end;

procedure typeconvtest;
    var 
        a: real;
        n: integer;

    procedure print (b: real);
        begin
            writeln ('para b = ', b:5:2)
        end;

    begin
        n := 2;
        a := n;
        writeln ('a = ' , a:5:1);
        print (5);
        print (n);
        print (-5);
        print (-n)
    end;

procedure typetest;

    procedure print (n: int64);
        begin
            writeln ('n = ', n)
        end;

    procedure printbyte (n: byte);
        begin
            writeln ('n = ', n)
        end;

    procedure printshortint (n: shortint);
        begin
            writeln ('n = ', n)
        end;

    procedure printword (n: word);
        begin
            writeln ('n = ', n)
        end;

    procedure printsmallint (n: smallint);
        begin
            writeln ('n = ', n)
        end;

    procedure printcardinal (n: cardinal);
        begin
            writeln ('n = ', n)
        end;

    procedure printinteger (n: integer);
        begin
            writeln ('n = ', n)
        end;

    procedure test;
        const 
            big = 1000 * 1000 * 1000 * 10;
        var 
            a: byte;
            b: shortint;
            c: word;
            d: smallint;
            e: cardinal;
            f: integer;
            g: int64;

        begin
            a := 1;
            b := -1;
            c := 257;
            d := -257;
            e := 32769;
            f := -32769;
            g := big;
            print (a);
            printbyte (a);
            print (b);
            printshortint (b);
            print (c);
            printword (c);
            print (d);
            printsmallint (d);
            print (e);
            printcardinal (e);
            print (f);
            printinteger (f);
            print (g)
        end;

    begin
        test
    end;

procedure writetest;
    var 
        a: real;
        c: char;
        n: integer;
        s: string;
        b: boolean;
        pi: real;
        i: integer;

    function f: string;
    begin
        f := 'ABC'
    end;

    function g: integer;
    begin
        g := 12345
    end;

    begin
        a := 4000 * arctan (1.0);
        c := chr (65);
        n := 12345;
        s := 'Hello, world';
        b := false;
        pi := 4 * arctan (1);

        writeln (a:10:4);
        writeln (c, ' ', c:1, ' ', c:20);
        writeln (n, ' ', n:1, ' ', n:20);
        writeln (s, ' ', s:1, ' ', s:30);
        writeln (b, ' ', b:1, ' ', b:30);

        for i := 2 to 10 do
            writeln ('pi = ', pi:2 * i:i);

	writeln (f);
        writeln (g)
    end;

procedure arithmetictest;
    var 
        a, b: integer;
        c, d, e, f, g, h, x, y: real;

    function exp_series (x: real): real;
        const
           eps = 0.00000001;
        var 
            res, add: real;
            n: integer;
        begin
            add := 1.0;
            res := 1.0;
            n := 0;
            repeat
                n := n + 1;
                add := add * x / n;
                res := res + add
            until abs (add) < eps;
            exp_series := res
        end;

    procedure integertest;
        var
            a, b, c, d: integer;
        begin
            a := 30; b := 40; c := 50; d := -30;
            writeln (a + b:8, a - b:8, a * b:8, c div a:8, c mod a:8, a / b:8:5);
            writeln (a and b:8, a or b:8, a xor b:8, not a:8);
            writeln (a < b:8, a > b:8, a <= c:8, a >= c:8, a <> b:8, a = d:8);
            writeln (b < a:8, b > a:8, c <= a:8, c >= a:8, a <> a:8, a = a:8)
        end;

    procedure realtest;
        var
            a, b, c, d: real;
        begin
            a := 30; b := 40; c := 50; d := -30;
            writeln (a + b:8:3, a - b:8:3, a * b:8:3, c / a:8:3);
            writeln (a < b:8, a > b:8, a <= c:8, a >= c:8, a <> b:8, a = d:8);
            writeln (b < a:8, b > a:8, c <= a:8, c >= a:8, a <> a:8, a = a:8)
        end;

    begin
        a := 11;
        b := 4;
        writeln ('a/b = ', a / b:10:5);
        writeln ('a div b = ', a div b);
        writeln ('a mod b = ', a mod b);

        x := 2.0;
        y := a + x * (b + 1);
        writeln ('y = ' , y:5:2);

        c := 5; d := 7; e := 9; f := 11; g := 13; h := 15; 
        y := -(c - (-d)) - (-(e - f) - (-g - h));
        writeln ('y = ' , y:10:3);
        y := -(c * (-d)) / (-(e + f) - (-g / h));
        writeln ('y = ' , y:10:3);

        writeln ('exp(1) = ', exp (1):10:7, ', exp_series (1) = ', exp_series (1):10:7);
        writeln ('exp(-1) = ', exp (-1):10:7, ', exp_series (-1) = ', exp_series (-1):10:7);

        integertest;
	realtest
    end;

procedure pointertest;
    type 
        intptr = ^integer;
        fwdptr = ^field;
        field = array [boolean] of char;
        rec = record
            a, b: integer
        end;
    var  
        p: intptr;
        q: ^char;
        a: integer;
        b: fwdptr;
	r: rec;
        pr: ^rec;
    begin
        p := nil;
        new (p);
        p^ := 2;
        a := p^;
        writeln ('a = ' , a);
        dispose (p);

        p := addr (a);
        p^ := 3;
        writeln ('a = ' , a);

        new (b);
        q := addr (b^[true]);
        q^ := chr (65);
        writeln ('b^[true] = ', b^[true]);
        dispose (b);

	pr := addr (r);
	pr^.a := 5;
        writeln (r.a);

	p := addr (r.a);
        p^ := 6;
        writeln (r.a)
    end;

procedure treetest;
    type 
        element = integer;
        nodeptr = ^node;
        node = record
            data: element;
            left, right: nodeptr
        end;
    var 
        root: nodeptr;

    procedure insertTree (var p: nodeptr; val: element);
        begin
            if p = nil then 
                begin
                    new (p);
                    p^.data := val;
                    p^.left := nil;
                    p^.right := nil
                end 
            else if val < p^.data then
                insertTree (p^.left, val)
            else
                insertTree (p^.right, val)
        end;

    procedure printTree (p: nodeptr);
        begin
            if p <> nil then 
                begin
                    printTree (p^.left);
                    write (p^.data, ' ');
                    printTree (p^.right)
                end
        end;

    procedure deleteTree (p: nodeptr);
        begin
            if p <> nil then 
                begin
                    deleteTree (p^.left);
                    deleteTree (p^.right);
                    dispose (p)
                end
        end;

    begin
        root := nil;
        insertTree (root, 5);
        insertTree (root, 2);
        insertTree (root, 9);
        insertTree (root, 7);
        printTree (root);
        writeln;
        deleteTree (root)
    end;
    
procedure rangechecktest;
    type  
        subrange = 1..10;

    procedure test;
        var
            a: -10..30;
            b: integer;
            d: word;
            c: int64;
            f: array [1..5] of -7..-3;

        procedure looptest;

            procedure routinetest (n: subrange);
                begin
                    writeln (n)
                end;

            var 
                a: -10..30;

            begin
                for a := 1 to 10 do
                    routinetest (a);
                f [3] := -5;
            end;

        procedure assigntest;

            function func: subrange;
                begin
                    func := 3
                end;

            begin
                a := func;
                a := 15;
                b := a;
                d := a;
                a := b;
                c := a;
                c := b
            end;

        begin
            looptest;
            assigntest
        end;

    begin
        test
    end;

procedure redeftest;
    var 
        a: integer;

    procedure proc1;
        begin
            writeln ('External a = ', a)
        end;

    procedure proc2;
        procedure proc3;
            begin
                a := 1;
                proc1
            end;

        var 
            a: integer;

        procedure proc1;
            begin
                writeln ('Internal a = ', a)
            end;

        begin
            a := 2;
            proc3;
	    proc1
        end;
 
    begin
        proc2
    end;

procedure deeprectest;
    const 
        n = 25 * 1000;
    type 
        para = 0..n + 1;
    var
        count: para;

    procedure proc (i: para; var count: para);
        begin
            count := count + 1;
            if i > 0 then
                proc (i - 1, count)
            else
                writeln (count, ' calls')
        end;

    begin
        count := 0;
        proc (n, count)
    end;

procedure retvaltest;
    type 
        field = array [1..100] of integer;
        rec = record
            n: integer;
            f: boolean;
            a: real
        end;
        sfield = array [1..100] of 
            record
                f: boolean;
                s: string
            end;

    function f1: field;
        var 
            i: integer;
            f: field;
        begin
            for i := 1 to 100 do
                 f [i] := 2 * i;
            f1 := f
        end;

    function f2 (n: integer): rec;
        var r: rec;
        begin
            r.n := n + 3;
            r.f := true;
            r.a := 1.0;
            f2 := r
        end;

    function f3 (a: integer): integer;
        begin
            f3 := a + 1
        end;

    function f4: sfield;
        var
            i: integer;
            f: sfield;
        begin
            for i := 1 to 100 do
                begin
                    f [i].s := 'Hello, world';
                    f [i].f := true
                end;
            f4 := f
        end;

    function f5: string;
        begin
            f5 := 'Hallo, world'
        end;

    function f6: integer;
        procedure nested;
            begin
                f6 := 54321
            end;

        begin
            nested
        end;

    var 
        a: field;
        r: rec;

    begin
        writeln (f3 (5));
        a := f1;
        writeln (a [10]);
        r := f2 (5);
        writeln (r.n);
        writeln (f1 [10]);
        writeln (f2 (5).n);
        writeln (f4 [10].s);
        writeln (f5);
        writeln (f6)
    end;

procedure forwardtest;
    procedure test1 (n: integer); forward;

    procedure test (n: integer);
    begin
        test1 (n)
    end;

    procedure test1 (n: integer);
        procedure nested (a, b: integer);
        var 
            i: integer;
        begin
            for i := a to b do
                write (i, ' ')
        end;
        
    begin
        nested (1, n);
        nested (3, n)
    end;

    begin
        test (5);
        test (10);
        writeln
    end;

procedure zahlsorttest;
    type
	str	= string;
	baum	= ^wurzel;
	wurzel	= record
     	    eintrag : str;
	    links, rechts : baum
        end;

    var
	Tab0bis19  	: array [0..19] of str;
	Tab20bis90	: array [2..9]  of str;
	SortBaum	: baum;
	Zaehler, Anzahl	: integer;

    procedure Initialisiere;
	begin
	    Tab0bis19 [ 0] := '';	    Tab0bis19 [ 1] := 'ein';
	    Tab0bis19 [ 2] := 'zwei';	    Tab0bis19 [ 3] := 'drei';
	    Tab0bis19 [ 4] := 'vier';	    Tab0bis19 [ 5] := 'fuenf';
	    Tab0bis19 [ 6] := 'sechs';	    Tab0bis19 [ 7] := 'sieben';
	    Tab0bis19 [ 8] := 'acht';	    Tab0bis19 [ 9] := 'neun';
	    Tab0bis19 [10] := 'zehn';	    Tab0bis19 [11] := 'elf';
	    Tab0bis19 [12] := 'zwoelf';	    Tab0bis19 [13] := 'dreizehn';
	    Tab0bis19 [14] := 'vierzehn';   Tab0bis19 [15] := 'fuenfzehn';
	    Tab0bis19 [16] := 'sechzehn';   Tab0bis19 [17] := 'siebzehn';
	    Tab0bis19 [18] := 'achtzehn';   Tab0bis19 [19] := 'neunzehn';

	    Tab20bis90 [2] := 'zwanzig';    Tab20bis90 [3] := 'dreissig';
	    Tab20bis90 [4] := 'vierzig';    Tab20bis90 [5] := 'fuenfzig';
	    Tab20bis90 [6] := 'sechzig';    Tab20bis90 [7] := 'siebzig';
	    Tab20bis90 [8] := 'achtzig';    Tab20bis90 [9] := 'neunzig'
	end;

    function ZahlWort (Zahl: word): string;
	var 
            e, z : 0..9;
            Wort: str;
	begin
	    if Zahl >= 1000 then 
                Wort := Tab0bis19 [Zahl div 1000] + 'tausend'
	    else 
                Wort := '';
	    Zahl := Zahl mod 1000;
	    if Zahl >= 100 then 
                Wort := Wort + Tab0bis19 [Zahl div 100] + 'hundert';
	    Zahl := Zahl mod 100;
	    z := Zahl div 10;
	    e := Zahl mod 10;
	    if z >= 2 then
		begin
		    if e <> 0 then Wort := Wort + Tab0bis19[e] + 'und';
		    Wort := Wort + Tab20bis90 [z] 
                end
	    else
	        if Zahl <> 1 then 
                    Wort := Wort + Tab0bis19 [Zahl]
		else 
                    Wort := Wort + 'eins';
	    ZahlWort := Wort
	end;

    procedure FuegeEin (Element: str; var Teilbaum: baum);
	begin
	    if Teilbaum = nil then 
                begin
		    New (Teilbaum);
		    Teilbaum^.rechts := nil;
		    Teilbaum^.links := nil;
		    Teilbaum^.eintrag := Element
                end
	    else if Teilbaum^.Eintrag < Element then
                FuegeEin (Element, Teilbaum^.rechts)
	    else 
                FuegeEin (Element, Teilbaum^.links)
	end;

    procedure BaumAusgabe (SortBaum: baum);
	begin
	    if SortBaum <> nil then 
                begin
		    BaumAusgabe(SortBaum^.links);
		    WriteLn(Zaehler:5, '  ', SortBaum^.Eintrag);
		    Zaehler := Zaehler + 1;
		    BaumAusgabe(SortBaum^.rechts) 
                end
	end;

    procedure ErzeugeBaum (var SortBaum: baum; Anzahl: integer);
	var 
            i: word;
	begin
	    for i := 1 to Anzahl do 
                FuegeEin (ZahlWort (i), SortBaum) 
	end;

    procedure LoescheBaum (SortBaum: baum);
        begin
            if SortBaum <> nil then begin
                LoescheBaum (SortBaum^.links);
                LoescheBaum (SortBaum^.rechts);
                dispose (SortBaum)
            end
        end;

    begin
	Initialisiere;
        Anzahl := 150;
	WriteLn ('Liste der alphabetisch sortierten Zahlworte zwischen 1 und ', Anzahl);
	WriteLn;
	SortBaum := nil;
	ErzeugeBaum (SortBaum, Anzahl);
	Zaehler := 1;
	BaumAusgabe (SortBaum);
        LoescheBaum (SortBaum)
    end;

procedure enumtest;
    const
        n = 10;

    type
        enum = (a, b, c, d);

    var 
        v: enum;
        i, j: 0..n;

    begin
        v := a;
        v := pred (succ (succ (v)));
        writeln ('ord (v) = ', ord (v));
        writeln (succ (false), ' ', pred (true), ' ', succ (5), ' ', pred (0), ' ', succ ('A'), ' ', pred ('Z'));
        writeln (ord (1000000), ' ', ord ('A'), ' ', ord (true));
        i := 0;
        j := n;
        while i < n do
            begin
                i := succ (i);
                write (i, ' ', j, ' - ');
                j := pred (j)
            end;
        writeln;
        writeln (chr (65), chr (ord ('A')));
        for v := a to d do 
            write (ord (v):5);
        writeln
    end;

procedure parametertest;
    procedure nested1 (a: byte; b: boolean; s: string; d: smallint; e: word);
        function nested2: string;
            procedure nested3;
                begin
                    nested2 := 'Result';
                    writeln (a, ' ', b, ' ', s, ' ', d, ' ', e)
                end;

	    begin
                nested3
            end; 

	begin
            writeln (nested2)
        end;

    begin
        nested1 (1, true, 'Hello', -4, 12346)
    end;

procedure stringfunctest;
    procedure test (s: string; a, b: integer);
        var 
            t, u: string;
        begin
            t := s;
            u := s;
            delete (t, a, b);
            insert ('XXX', u, a);
            writeln (a:5, b:5, ' Copy: ', copy (s, a, b):10 , ' Delete: ', t:10, ' Insert: ', u: 15);
        end;

    var
        s, t: string;

    begin
        s := 'Hello';
        t := 'll';
        writeln (length (s));
        writeln (pos (t, s));
        writeln (pos ('foo', 'bar'));

        test (s, 2, 3);
        test (s, 4, 4);
        test (s, 5, 1);
        test (s, 5, 2);
        test (s, -10, 5);
        test (s, 10, 10);
        test (s, 2, 0);
        test (s, 2, -10);
        test (s, -10, -10);
    end;

procedure stringindex;
    var 
        s: string;
        i: integer;

    procedure toupper (var c: char);
        begin
            if (c >= 'a') and (c <= 'z') then
                c := chr (ord (c) - ord ('a') + ord ('A'))
        end;

    function tolower (c: char): char;
        begin
            if (c >= 'A') and (c <= 'Z') then
                tolower := chr (ord (c) - ord ('A') + ord ('a'))
            else
                tolower := c
        end;

    begin
        s := 'Hello';
        for i := 1 to length (s) do
            write (s [i], ' ');
        writeln;
        for i := 1 to length (s) do
            toupper (s [i]);
        writeln (s);
        for i := 1 to length (s) do
            s [i] := tolower (s [i]);
        writeln (s)
    end;

begin
    writeln (' ** simpletest');    	simpletest;
    writeln (' ** doubletest');		doubletest;
    writeln (' ** fortest');		fortest;
    writeln (' ** unarytest');		unarytest;
    writeln (' ** recordtest');		recordtest;
    writeln (' ** stringtest');		stringtest;
    writeln (' ** stringchartest');	stringchartest;
    writeln (' ** arraytest');		arraytest;
    writeln (' ** quicksorttest');	quicksorttest;
    writeln (' ** refpartest');		refpartest;
    writeln (' ** factest');		factest;
    writeln (' ** typeconvtest');	typeconvtest;
    writeln (' ** typetest');		typetest;
    writeln (' ** writetest');		writetest;
    writeln (' ** arithmetictest');	arithmetictest;
    writeln (' ** pointertest');	pointertest;
    writeln (' ** treetest');		treetest;
    writeln (' ** rangechecktest');	rangechecktest;
    writeln (' ** redeftest');		redeftest;
    writeln (' ** deeprectest');	deeprectest;
    writeln (' ** retvaltest');		retvaltest;
    writeln (' ** forwardtest');	forwardtest;
    writeln (' ** zahlsorttest');	zahlsorttest;
    writeln (' ** enumtest');		enumtest;
    writeln (' ** parametertest');	parametertest;
    writeln (' ** stringfunctest');	stringfunctest;
    writeln (' ** stringindex');	stringindex;
    writeln ('Regression tests done')
end.
