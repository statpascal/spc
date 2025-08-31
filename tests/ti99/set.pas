program settest;

procedure writeset (msg: string [5]; s: set of char);
    var
        ch: char;
    begin
        write (msg:5, ' ');
        for ch := #32 to #127 do
            if ch in s then
                write (ch);
        writeln
    end;
    
var 
    a, b, c: set of char;
    ch1, ch2: char;
    
begin
    a := ['A'..'J'];
    b := ['G'..'M'];
    writeset ('A', a);
    writeset ('B', b);
    writeset ('A*B', a * b);
    writeset ('B+Z', b + ['Z']);
    writeset ('B+a-f', b + ['a'..'f']);
    ch1 :='A'; ch2 := 'C';
    c := [ch1..ch2, 'X'..'Z'];
    writeset ('C', c);
    writeset ('A-C', a - c);
    writeset ('B+C', b + c);
    writeln ('A=A   ', a = a);
    writeln ('A<>A  ', a <> a);
    writeln ('A<A   ', a < a);
    writeln ('A<=A  ', a <= a);
    writeln ('A>A   ', a > a);
    writeln ('A>=A  ', a >= a);
    writeln ('A<A+B ', a < a + b);
    writeln ('A<A*B ', a < a * b);
    waitkey
end.
