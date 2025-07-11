program dummy;

const n = 10;

var a: ^integer;
    s: ^string;
    t: string;
    i: 0..n;

begin
    new (a, n);
    new (s, n);
    for i := 0 to n - 1 do begin
        a [i] := i + 1;
        str (a [i], t);
        s [i] := 'Hello, world #' + t
    end;
    for i := 0 to n - 1 do
        writeln (a [i], ': ', s [i]);

    dispose (a);
    dispose (s);
end.
