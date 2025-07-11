program randomtest;

procedure print;
    const 
        count = 10;
        max = 10;
    var
        i: integer;
    begin
        for i := 1 to count do
            write (random (max): 3);
        writeln;
        for i := 1 to count do
            write (random:10:7);
        writeln
    end;

begin
    print;
    randomize;
    print;
    randomize (1);
    print;
end.

        