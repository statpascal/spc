program extern;

const
    libname = 'external.so';

type 
    rec = record
        a: integer;
        b: int64;
        c: char;
        d: real;
        e: boolean;
        f: record
            g: byte;
            h: word
        end 
    end; 
    intptr = ^integer;
    charptr = ^char;

function add (a, b: integer): integer; external libname;
function multiply (a, b: integer): integer; external libname name 'mul';

procedure inc (var a: integer); external libname;
procedure incptr (b: intptr); external libname name 'inc';

procedure printrec (var r: rec); external libname;
procedure changerec (var r: rec); external libname;

procedure printstring (p: charptr); external libname;
procedure printstring1 (s: string); external libname;

procedure setstring (var s: string); external libname;
procedure modifystring (var s: string); external libname;

function sub (a, b: integer): integer;
    begin
        sub := a - b
    end;

var 
    a: integer;
    r: rec;
    s: string;
    i: int64;

begin
    writeln (add (2, 3));
    writeln (multiply (6, 7));
    writeln (sub (2, 3));
    a := 5;
    inc (a);
    writeln (a);
    incptr (addr (a));
    writeln (a);
    r.a := 1;
    r.b := -5;
    r.c := 'A';
    r.d := 4.0 * arctan (1.0);
    r.e := false;
    r.f.g := 31;
    r.f.h := 71;
    printrec (r);
    changerec (r);
    writeln (r.a, ' ', r.b, ' ', r.c, ' ', r.d, ' ', r.e, ' ', r.f.g, ' ', r.f.h);
    s := 'Hello world';
    printstring (addr (s [1]));
    printstring1 (s);
    writeln;
    setstring (s);
    writeln (s);
    modifystring (s);
    writeln (s)
end.
