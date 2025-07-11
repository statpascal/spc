program hillvec;

const 
    alpha = 2.0;
    nmax = 10000;

function hill (x: realvector): realvector;
    var 
	n: integer;
    begin
	x := log (rev (sort (x [x > 0])));
	n := size (x);
	hill := 1.0 / (cumsum (x) [intvec (1, n - 1)] / intvec (1, n-1)  - x [intvec (2, n)])
    end;

procedure test;
    var 
        x, y: realvector;
    begin
	randomize (0);
	x := power (randomdata (nmax), -1.0 / alpha);
   	y := hill (x);
	writeln (y)
    end;

begin
    test
end.
