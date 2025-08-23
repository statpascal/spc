program qsort;

procedure quicksorttest;
    const 
        n = 150;
    type 
        element = integer;
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
            for i := 1 to n do 
		a [i] := n + 1 - i;
            qsort (a, 1, n);
            for i := 1 to n do 
		write (a [i]:4);
            writeln
        end;
            
    begin
        test
    end;

begin
    quicksorttest;
    waitkey
end.

