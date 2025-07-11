program avltest;

uses avl, math;

(* FPC needs math for sign function *)

procedure print (s: pointer);
    type
	charptr = ^char;
    var
	t: charptr absolute s;
    begin
	while t^ <> chr (0) do
	    begin
		write (t^);
		inc (t)
	    end
    end;

function strcmp (a, b: pointer): integer;
    type
	byteptr = ^byte;
    var
	s: byteptr absolute a;
        t: byteptr absolute b;
        res: integer;
	done: boolean;
    begin
	repeat
            res := sign (s^ - t^);
	    done := (res <> 0) or (t^ = 0);
	    inc (s); inc (t)
        until done;
	strcmp := res
    end;

procedure Ausgabe(Baum: AVLBaum);
    begin
	if Baum <> nil then begin
	    Ausgabe(Baum^.L);
	    print (Baum^.KeyPtr);
	    writeln;
	    Ausgabe(Baum^.R)
	end
    end;

const
    n = 100;

var 
    Baum: AVLBaum;
    a: string;
    i: 1..n;
    

begin
    Baum := nil;
    for i := 1 to n do
	begin
	    str (i, a);
      	    fuegeein (Baum, nil, addr (a [1]), 0, Succ(Length(a)), strcmp)
	end;
    Ausgabe(Baum)
end.