program valtest;

var
    s: string;
    a: int64;
    b: real;
    c: int32;
    d: uint8;
    e: single;
    code: word;

begin
    s := '1234567';
    val (s, a, code);
    writeln ('s = ', s, ', ival = ', a, ', code = ', code);
    val (s, c, code);
    writeln ('s = ', s, ', ival = ', c, ', code = ', code);

    s := '123';
    val (s, d, code);
    writeln ('s = ', s, ', ival = ', d, ', code = ', code);
    
    s := '12345.67';
    val (s, a, code);
    writeln ('s = ', s, ', ival = ', a, ', code = ', code);
    val (s, b, code);
    writeln ('s = ', s, ', fval = ', b:10:2, ', code = ', code);
    val (s, e, code);
    writeln ('s = ', s, ', fval = ', e:10:2, ', code = ', code);

    s := '1234,567';
    val (s, b, code);
    writeln ('s = ', s, ', fval = ', b:10:2, ', code = ', code);
end.
