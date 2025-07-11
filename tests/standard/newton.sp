program Newton(input, output);

    (* Newtonverfahren zur Nullstellenbestimmung *)

    uses Rechner;

    var  xWert : real;
	 f     : string;

    procedure Variable(a: string; var Erfolg: boolean; var Erg: real);
	begin
	    if a = 'X'
		then begin
		    Erfolg := true;
		    Erg := xWert end
		else Erfolg := false
	end (* Variable *);

    function fs(x:real): real;
	const h = 1e-5;
	var   x1, x2: real;
	      Dummy : FehlerTyp;
	begin
	    xWert := x + h;
	    Berechne(f, x2, Dummy, Variable);
	    xWert := x;
	    Berechne(f, x1, Dummy, Variable);
	    fs := (x2 - x1) / h
	end;

    procedure Iteration;
	const Epsilon = 1e-8;
	var x, xn, fx: real;
	    Dummy : FehlerTyp;
	    ch : char;
	begin
	    repeat
		Write('f(x) = ');
		ReadLn(f);
		repeat
		    Write('x1 = ');
		    ReadLn(xn);
		    repeat
			x := xn;
			xWert := x;
			Berechne(f, fx, Dummy, Variable);
			xn := x - fx/fs(x)
		    until Abs(x - xn) < Epsilon;
		    WriteLn('Nullstelle bei x = ', xn:10:6);
		    Write('Weitere Iteration (j/n)?  ');
		    ReadLn(ch)
		until (ch = 'n') or (ch = 'N');
		Write('Weitere Funktion (j/n)? ');
		ReadLn(ch)
	    until (ch = 'n') or (ch = 'N')
        end;

    begin
	Iteration
    end.