program runtimetest;

var 
    x: vector of real;
    s: string;

begin
    x := intvec (1, 10);
    writeln ('sqrt (x): ', sqrt (x));
    writeln ('sin (x):  ', sin (x));
    writeln ('cos (x):  ', cos (x));
    writeln ('log (x):  ', log (x));
    randomize (0);
    writeln ('perm (10): ', randomperm (10));
    s := 'Hello, world';
    insert ('to the ', s, 7);
    writeln (s);
    writeln (length (s));
    writeln ('POS: ', pos ('b', 'abc'));
    writeln (copy (s, 1, 5));
    delete (s, 1, 5);
    writeln (s);
    x := realvec (0, 7, 0.2);
    writeln (sin (x) * sin (x) + cos (x) * cos (x))
end.
