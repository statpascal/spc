program displaytest;

{$bank:on}

procedure p1;
    var
        a: integer;

    procedure p2;
        begin
            a := 5
        end;

    procedure p3;
        begin
            a := 3;
            p2
        end;

    begin
        a := 1;
        p3;
        writeln ('Should print 5: ', a)
    end;

{$bank:off}

begin
    p1;
    waitkey
end.
