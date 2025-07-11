program realveccomp;

const
    n = 20;

var 
    a: vector of real;

begin
    a := intvec (1, n);

    writeln (a);
    writeln ('<  ', a < 10);
    writeln ('>  ', a > 10);
    writeln ('=  ', a = 10);
    writeln ('<> ', a <> 10);
    writeln ('<= ', a <= 10);
    writeln ('>= ', a >= 10)
end.
