program strchar;

var
    ch: char;
    s, t: string;

function f: char;
    begin
	f := 'b'
    end;

begin
    ch := 'x';
    s := ch;
    writeln (s);
    s := upcase (ch);
    writeln (s);
    s := f;
    writeln (s);
    s := 'A';
    writeln (s);
    t := ch + s;
    writeln (t);
    t := s + ch;
    writeln (t);
    t := t + upcase (f);
    writeln (t)
end.
