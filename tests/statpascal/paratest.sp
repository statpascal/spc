program paratest;

const
    libname = 'paratest.so';
    n = 100;

type 
    rec = record
	a: int64;
	s1: string;
	b: real;
	s2: string;
	c: char
    end;
    arr = array [1..n] of integer;

function manypara (a, b, c, d, e, f, g, h, i, j: integer): integer; external libname;
function manyparastr (a, b, c, d, e, f, g, h, i, j: string): string; external libname;

function manypara1 (a, b, c, d, e, f, g, h, i, j: integer): integer;
    begin
	writeln (a, b, c, d, e, f, g, h, i, j);
	manypara1 := h
    end;

function manystr (a, b, c, d, e, f, g, h, i, j: string): string;
    begin
	writeln (a, b, c, d, e, f, g, h, i, j);
	manystr := i
    end;

function manyrec (a, b, c, d, e, f, g, h, i, j: rec): rec;
    begin
	manyrec := j
    end;

function manyarr (a, b, c, d, e, f, g, h, i, j: arr): arr;
    begin
	manyarr := h
    end;

procedure dotest;
    var
        a: integer;
        s: string;
	i: 0..9;
	j: 1..n;
	r: array [0..9] of rec;
	r1: rec;
	f: array [0..9] of arr;
	f1: arr;

    function func (i: integer): rec;
	begin
	    func := r [i]
	end;

    begin
        a := manypara (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        writeln (a);
        a := manypara1 (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
        writeln (a);
        s := manystr ('S1', 'S2', 'S3', 'S4', 'S5', 'S6', 'S7', 'S8', 'S9', 'S10');
        writeln (s);
        s := manyparastr ('S1', 'S2', 'S3', 'S4', 'S5', 'S6', 'S7', 'S8', 'S9', 'S10');
        writeln (s);
	for i := 0 to 9 do
	    with r [i] do
		begin
		    a := i;
		    str (i, s1);
		    b := i + 0.1;
		    str (i, s2);
		    c := chr (i + 65)
		end;
	r1 := manyrec (r [0], r [1], r [2], r [3], r [4], r [5], r [6], r [7], r [8], r [9]);
	writeln (r1.s2);
	r1 := manyrec (func (0), func (1), func (2), func (3), func (4), func (5), func (6), func (7), func (8), func (9));
	writeln (r1.s2);
	for i := 0 to 9 do
	    for j := 1 to n do
		f [i, j] := i * n + j;
	f1 := manyarr (f [0], f [1], f [2], f [3], f [4], f [5], f [6], f [7], f [8], f [9]);
	writeln (f1 [50]);
	writeln (manyarr (f [0], f [1], f [2], f [3], f [4], f [5], f [6], f [7], f [8], f [9]) [34])
    end;

begin
    dotest
end.
