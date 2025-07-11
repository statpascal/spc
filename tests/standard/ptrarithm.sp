var
    a: array [1..2] of integer;
    p, q: ^integer;
    p1, p2: pointer;

begin
    p := addr (a [1]); q := addr (a [2]);
    writeln (q - p);
    q := p; inc (q);
    writeln (q - p);
    writeln (q > p, ' ', q < p, ' ', q <= p, ' ', q >= p, ' ', q = p, ' ', q <> p);

(*    
    Not supported by FPC in Delphi mode:

    writeln ((p + 5) - p); 
*)

    p1 := p; p2 := q;
    writeln (p2 - p1);
end.
