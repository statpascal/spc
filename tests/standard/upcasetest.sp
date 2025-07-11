program upcasetest;

var
    ch: char;
    s: string;

begin
    ch := 'a';
    s := 'abcxyz';
    writeln (upcase (ch), ' ', upcase (s))
end.