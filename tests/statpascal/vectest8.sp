program test;

procedure p;
var 
    v, u: int64vector;
    w: realvector;

begin
    randomize (0);
    v := intvec (1, 20);
    u := v + v;
    writeln (u);
   
    w := realvec (0.1, 2, 0.1);
    writeln (w);
    writeln (w + w);

    writeln (randomperm (10));
    writeln (randomdata (10));
    writeln (randomdata (10, 10));
    writeln (sqrt (w));
    writeln (sin (w));
    writeln (cos (w));
    writeln (log (w));
    writeln (power (w, 2));

end;

begin p end.
