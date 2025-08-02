program hello;

var
    s: string [20];
    i: integer;

begin
    s := 'Hello, world 1!';
    writeln (s);
    writeln ('Hello, world 2!');
    for i := 1 to 10 do
        writeln ('Hello', i:5);
    waitkey
end.