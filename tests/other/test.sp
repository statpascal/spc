program test;

const x = 42;
      ys = 'String'; 
      pi = 3.1415926;
      y = x + 5 * pi;

var i, a, b: integer;
    s: string;
    c: real;
    d: array [char] of real;
    e: array [char, boolean] of string;

procedure test1;
var c, d: real;
begin
    c := d
end;

function test2 (var a, b: integer; var c: real; d, e: string): integer;
var x, y: real;
begin
    if a < b then
        a := b;
    c := b;
    c := a;
    c := c;
    a := a;
    c := x;
    y := c
end;

procedure test3 (var s: string);
begin
    s := 'Hello'
end;

begin
    test1;
    a := test2 (a, b, c, s, s);

    test3 (s);

    d ['A'] := 3.14;

    repeat
        b := 5;
        a := b
    until b = 5;
    for i := 1 to 10 do begin 
        a := 42;
        c := b
    end;
    if a < b then
        while s <> 'Hello' do 
            s := 'Hello'
    else begin
        a := 3;
        b := a
    end;
    c := a + a * (b + 3)
end.

(* Comment at end of program *)
