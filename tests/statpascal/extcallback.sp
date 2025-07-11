program extcallback;

const
    libname = 'extcallback.so';

type
    charptr = ^char;
    func1 = function (a: byte; b: word; c: integer; d: int64; e: real; f: charptr): byte;
    func2 = function (var a: byte; var b: word; var c: integer; var d: int64; var e: real; var f: charptr): real;
    func3 = function (a: integer; b: smallint): integer;
    proc1 = procedure;
    proc2 = procedure (n: integer);

var
    a: byte;
    b: word;
    c: integer;
    d: int64;
    e: real;
    f: charptr;

procedure callbacktest (f1: func1; f2: func2; p1: proc1; p2: proc2); external libname;

function f1 (apara: byte; bpara: word; cpara: integer; dpara: int64; epara: real; fpara: charptr): byte;
    begin
	a := apara;
        b := bpara;
	c := cpara;
	d := dpara;
	e := epara;
	f := fpara;
	f1 := 140;
    end;

function f2 (var apara: byte; var bpara: word; var cpara: integer; var dpara: int64; var epara: real; var fpara: charptr): real;
    begin
	apara := a;
	bpara := b;
	cpara := c;
	dpara := d;
	epara := e;
	fpara := f;
	f2 := exp (1)
    end;

procedure p1;
    begin
	writeln ('Procedure proc1')
    end;

procedure p2 (n: integer);
    begin
	writeln ('Procedure proc2, n = ', n)
    end;

procedure recursiontest (p: proc2; n: integer); external libname;

procedure p3 (n: integer);
    begin
        write (n: 5);
        if n > 0 then
	    recursiontest (p3, n)
	else
 	    writeln
    end;

function simpletest (a: integer; b: smallint; f: func3): integer; external libname;

function add (a: integer; b: smallint): integer;
    begin
	add := a + b
    end;

procedure p4;
    const 
        n = 1000;
    var 
        i: 1..n;
        sum: int64;
    begin
	sum := 0;
	for i := 1 to n do
	    sum := sum + simpletest (2, 3, add);
        writeln (sum)
    end;

begin
    callbacktest (f1, f2, p1, p2);
    p3 (1000);
    p4
end.
