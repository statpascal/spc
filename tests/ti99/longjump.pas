program longjump;

procedure test1;    
    var
        jmpbuf: jmp_buf;
    begin
        if Setjmp (jmpbuf) = 0 then
            begin
                writeln ('jmpbuf set');
                Longjmp (jmpbuf, 1)
            end
        else
            writeln ('Longjmp performed')
    end;
    
procedure test2;
    var
        val: integer;
        jmpbuf: jmp_buf;

    procedure recurse (n: integer);
        begin
            write (n:3);
            if n = 1 then
                longjmp (jmpbuf, 1)
            else
                recurse (pred (n))
        end;
        
    begin
        val := 5;
        if setjmp (jmpbuf) = 0 then
            recurse (10)
        else
            begin
                writeln;
                writeln ('Recursion terminated, val is ', val, ' (should be 5)')
            end
    end;
        
begin
    test1;
    test2;
{$ifdef ti99}    
    waitkey
{$endif}    
end.
