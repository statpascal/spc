program dummy;

procedure zahlsorttest;
    type
	str	= string;
	baum	= ^wurzel;
	wurzel	= record
     	    eintrag : str;
	    links, rechts : baum
        end;

    var
	Tab0bis19  	: array [0..19] of str;
	Tab20bis90	: array [2..9]  of str;
	SortBaum	: baum;
	Zaehler, Anzahl	: integer;

    procedure Initialisiere;
	begin
	    Tab0bis19 [ 0] := '';	    Tab0bis19 [ 1] := 'ein';
	    Tab0bis19 [ 2] := 'zwei';	    Tab0bis19 [ 3] := 'drei';
	    Tab0bis19 [ 4] := 'vier';	    Tab0bis19 [ 5] := 'fuenf';
	    Tab0bis19 [ 6] := 'sechs';	    Tab0bis19 [ 7] := 'sieben';
	    Tab0bis19 [ 8] := 'acht';	    Tab0bis19 [ 9] := 'neun';
	    Tab0bis19 [10] := 'zehn';	    Tab0bis19 [11] := 'elf';
	    Tab0bis19 [12] := 'zwoelf';	    Tab0bis19 [13] := 'dreizehn';
	    Tab0bis19 [14] := 'vierzehn';   Tab0bis19 [15] := 'fuenfzehn';
	    Tab0bis19 [16] := 'sechzehn';   Tab0bis19 [17] := 'siebzehn';
	    Tab0bis19 [18] := 'achtzehn';   Tab0bis19 [19] := 'neunzehn';

	    Tab20bis90 [2] := 'zwanzig';    Tab20bis90 [3] := 'dreissig';
	    Tab20bis90 [4] := 'vierzig';    Tab20bis90 [5] := 'fuenfzig';
	    Tab20bis90 [6] := 'sechzig';    Tab20bis90 [7] := 'siebzig';
	    Tab20bis90 [8] := 'achtzig';    Tab20bis90 [9] := 'neunzig'
	end;

    function ZahlWort (Zahl: word): string;
	var 
            e, z : 0..9;
            Wort: str;
	begin
	    if Zahl >= 1000 then 
                Wort := Tab0bis19 [Zahl div 1000] + 'tausend'
	    else 
                Wort := '';
	    Zahl := Zahl mod 1000;
	    if Zahl >= 100 then 
                Wort := Wort + Tab0bis19 [Zahl div 100] + 'hundert';
	    Zahl := Zahl mod 100;
	    z := Zahl div 10;
	    e := Zahl mod 10;
	    if z >= 2 then
		begin
		    if e <> 0 then Wort := Wort + Tab0bis19[e] + 'und';
		    Wort := Wort + Tab20bis90 [z] 
                end
	    else
	        if Zahl <> 1 then 
                    Wort := Wort + Tab0bis19 [Zahl]
		else 
                    Wort := Wort + 'eins';
	    ZahlWort := Wort
	end;

    procedure FuegeEin (Element: str; var Teilbaum: baum);
	var 
            Neu : baum;
	begin
	    if Teilbaum = nil then 
                begin
		    New (Neu);
		    Neu^.rechts := nil;
		    Neu^.links := nil;
		    Neu^.eintrag := Element;
		    Teilbaum := Neu 
                end
	    else
	        if Teilbaum^.Eintrag < Element then
		    FuegeEin (Element, Teilbaum^.rechts)
		else 
                    FuegeEin (Element, Teilbaum^.links)
	end;

    procedure BaumAusgabe (SortBaum: baum);
	begin
	    if SortBaum <> nil then 
                begin
		    BaumAusgabe(SortBaum^.links);
		    WriteLn(Zaehler:5, '  ', SortBaum^.Eintrag);
		    Zaehler := Zaehler + 1;
		    BaumAusgabe(SortBaum^.rechts) 
                end
	end;

    procedure ErzeugeBaum (var SortBaum: baum; Anzahl: integer);
	var 
            i: word;
	begin
	    for i := 1 to Anzahl do 
                FuegeEin (ZahlWort (i), SortBaum) 
	end;

    begin
	Initialisiere;
        Anzahl := 2;
	WriteLn ('Liste der alphabetisch sortierten Zahlworte zwischen 1 und ', Anzahl);
	WriteLn;
	SortBaum := nil;
	ErzeugeBaum (SortBaum, Anzahl);
	Zaehler := 1;
	BaumAusgabe (SortBaum)
    end;

begin
    zahlsorttest
end.