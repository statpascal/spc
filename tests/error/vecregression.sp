program vecregression;

procedure vectortest;
    procedure testbool;
        var 
            v1: vector of boolean;
            v2: vector of integer;
        begin
            v2 := intvec (1, 10);
            writeln (v2 > 5);
            v1 := v2 > 5;
            writeln (v1)
        end; 

    procedure testindex;
        var 
            v1, v2: vector of integer;
            v3: vector of boolean;
        begin
           v1 := intvec (1, 100);
           v2 := intvec (30, 40);
           writeln (v2 + 5);
           writeln (v1 [v2]);
           writeln (v1 [v2 + 5]);
           v3 := v1 mod 2 <> 0;
           writeln (v1 [v3])
        end;

    procedure testarithmetic;
        type
            intvector = vector of integer;
            rec = record
                a: integer;
                b: real;
                c: boolean;
                d: string;
                e: intvector
            end; 

        var 
            v1, v2: intvector;
            v3: vector of rec;
            v4: vector of string;
            r: rec;
            v5: vector of real;
            v6: vector of byte;
            v7: vector of word;
            v8: vector of cardinal;
            v9: vector of int64;

       begin
           v1 := intvec (1, 10);
           v2 := intvec (-5, 4);
           r.e := v1;
           writeln (v1);
           writeln (v2);
           writeln (r.e);

           writeln (v1 + v1);
           writeln (v2 * v2 > v1);
           writeln (v1 + 1);
           writeln (v1 >= 5);

           v1 := 5;
           v3 := r;

           v5 := 4.0 * arctan (1.0);
           writeln (v5);
           v5 := v2;
           writeln (v5 + 1.2);
           writeln (intvec (1, 10) / intvec (11, 20));
           writeln (intvec (40, 50) div intvec (10, 20));
           writeln (intvec (1, 20) mod 2);

           v6 := intvec (1, 10);
           v7 := v6;
           v8 := v7;
           v9 := v8;
           writeln (v9);
        end;

    procedure testcombine;
        var
            v1: vector of int64;
            v2: vector of byte;
        begin
            v1 := combine (1, 2, 3, 4, 5);
            writeln (v1);
            v1 := combine (intvec (1, 5), 12, intvec (10, 15));
            writeln (v1);
            v2 := intvec (50, 60);
            writeln (v2);
            v1 := combine (v1, v2);
            writeln (v1);
        end;

    begin
        testbool;
        testindex;
        testarithmetic;
        testcombine
    end;

procedure qsortvec;
    type 
        element = integer;
        intvector = vector of element;

    function qsort (a: intvector): intvector;
        var
            median: element;
        begin
            if size (a) <= 1 then
                qsort := a
            else
                begin
                    median := a [size (a) div 2];
                    qsort := combine (qsort (a [a < median]), a [a = median], qsort (a [a > median]))
                end
        end;

    var 
        a: intvector;

    begin
        a := combine (3, -7, 9, -1, 2, 5, 3, 17, -21);
        writeln (a);
        writeln (qsort (a))
    end;

procedure recursive;
    procedure simpletest;
        type
            rec = record
                a: integer;
                v: vector of real;
                s: string
            end;
            recvec = vector of rec;

        var
            v: recvec;
            e: rec;

        begin
            e.a := 1;
            e.v := combine (1.0, 2.0, 3.0);
            e.s := 'Hello';

            v := e;
            v := combine (v, e);
            v := combine (v, v);
            writeln (v [2].s);
            writeln (v [3].v);
        end;

    type 
        realmatrix = vector of vector of real;
        stringmatrix = vector of vector of string;

    procedure printmatrix (var m: realmatrix);
        var
            i, j: integer;
        begin
            for i := 1 to size (m) do
                begin
                    for j := 1 to size (m [i]) do
                        write (m [i, j]:3);
                    writeln
                end
        end;

    procedure printstringmatrix (var m: stringmatrix; outputlen: integer);
        var
            i, j: integer;
        begin
            for i := 1 to size (m) do
                begin
                    for j := 1 to size (m [i]) do
                        write (m [i, j]:outputlen);
                    writeln
                end
        end;

    procedure matrix1;
        type 
            realvector = vector of real;
        var
            m: realmatrix;
            v1, v2, v3: realvector;
        begin
            v1 := combine (1.0, 0.0, 0.0);
            v2 := combine (0.0, 1.0, 0.0);
            v3 := combine (0.0, 0.0, 1.0);

            m := v1;
            m := combine (m, v2, v3);
            printmatrix (m)
        end;

    procedure matrix2;
        var
            m: realmatrix;
        begin
            m := combine (1.0, 0.0, 0.0);
            m := combine (m, combine (0.0, 1.0, 0.0));
            m := combine (m, combine (0.0, 0.0, 1.0)); 
            printmatrix (m)
        end;

    procedure matrix3 (n: integer);
        var 
            m: realmatrix;
            v, empty: vector of real;
            i, j: integer;
        begin
            for i := 1 to n do begin
                v := empty;
                for j := 1 to n do
                    if i = j then
                        v := combine (v, 1)
                    else
                        v := combine (v, 0);
                m := combine (m, v)
            end;
            printmatrix (m)
        end;

    procedure matrix4 (n: integer);
        const
            outputlen = 6;
        type
            stringvector = vector of string;
        var
            m: stringmatrix;
            v, empty: stringvector;
            i, j: integer;
	
	function tostr (i: integer): string;
	    var
		s: string;
	    begin
		str (i, s);
		tostr := s
	    end;

        begin
            for i := 1 to n do begin
                v := empty;
                for j := 1 to n do
                    v := combine (v, tostr (i) + '-' + tostr (j));
                m := combine (m, v)
            end;
            printstringmatrix (m, outputlen)
        end;

    begin
        simpletest;
        matrix1;
        matrix2;
        matrix3 (3);
        matrix3 (10);
        matrix4 (10);
    end;   

begin
    vectortest;
    qsortvec;
    recursive
end.
