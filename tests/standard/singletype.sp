program singletype;

const
    n = 10;

var
    b, c: double;
    x, y, z: single;
    a: array [1..n] of single;
    i: 1..n;

function dbl_sgl (x: double): single;
    begin
	dbl_sgl := x
    end;

function sgl_dbl (x: single): double;
    begin
	sgl_dbl := x
    end;

function sgl: single;
    begin
	sgl := exp (1)
    end;

begin
    y := 3.1415;
    b := exp (1);
    x := y;
    z := x + y;
    writeln ('x = ', x:9:6, ', z = ', z:9:6);
    b := x;
    writeln ('b = ' , b:9:6);
    z := b * y;
    writeln ('z = ', z:9:6);
    writeln (sgl:7:5);
    b := dbl_sgl (x);
    writeln (b:7:5);
    x := sgl_dbl (b);
    writeln (x:7:5);
    for i := 1 to n do
        a [i] := sgl_dbl (i);
    for i := 1 to n do
        write (a [i]:5:1);
    writeln
end.
