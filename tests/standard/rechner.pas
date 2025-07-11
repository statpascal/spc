unit Rechner;

interface

    (*
       Berechnung eines Ausdrucks
       Eingabe:  a - Ausdruck (string)
       RÅckgabe: Erg - Ergebnis der Auswertung
		 Fehler - Fehlercode von Typ Fehlertyp

       Zum Zugriff auf Variablen kann eine Prozedur vom Typ VarProc
       mit folgenden Parametern Åbergeben werden:

       1. String - Name der Variable
       2. Boolean - Flag fÅr erfolgreiche Auswertung
       3. Real - Wert der Variablen

       Werden keine Variablen benîtigt, kann die unten aufgefÅhrte
       Prozedur DummyVarProc verwendet werden.
    *)

    type VarProc   = procedure(a: string; var b: boolean; var c: real);
	 FehlerTyp = (ENoError, EVar, ESyntax, ERange, EArg);

    procedure Berechne(a: string; var Erg: real; var Fehler: FehlerTyp; VarZugriff: VarProc);
    procedure DummyVarProc(a: string; var b: boolean; var c: real);

implementation

    (* Bem 1.: Mit Bem 1. gekennzeichnete Prozeduren erwarten die öbergabe
       des ersten auszuwertenden Zeichens in der globalen Variablen 'ch'.
       Nach AusfÅhrung der Prozedur befindet sich das erste nicht mehr
       bearbeitete Zeichen bereits in 'ch'. *)

    const Buchstabe = ['A'..'Z', 'a'..'z'];	(* Zeichen fÅr Bezeichner *)
	  Ziffer    = ['0'..'9'];

    procedure Berechne(a: string; var Erg: real; var Fehler: FehlerTyp; VarZugriff: VarProc);

	var ch : char;         	(* nÑchstes zu bearbeitendes Zeichen *)
	    Position : integer; (* Position im auszuwertendes String *)

		(* Position wird bei Aufruf initialisiert. VerÑnderung
		   dieser Variablen sonst ausschlie·lich in 'HoleZeichen'
		   bzw. 'NaechstesZeichen' *)

	    FehlerZustand : FehlerTyp; 	(* speichert Fehlerstatus, s.u. *)


	(* Setzen der globalen Variable FehlerZustand, erfolgt
	   ausschlie·lich Åber diese Prozedur *)

	procedure SetError(a: FehlerTyp);
	    begin
		if FehlerZustand = ENoError then FehlerZustand := a
	    end (* SetError *);

	(* gibt nÑchstes zu interpretierendes Zeichen aus Åbergebenem
	   String aus, bzw. '$' bei Erreichen des Ende. Leerzeichen
	   werden Åbersprungen *)

	function HoleZeichen : char;
	    begin
		while (Position <= Length(a)) and (a[Position] = ' ') do Inc(Position);
		if Position <= Length(a)
		    then begin
			HoleZeichen := a[Position];
			Inc(Position) end
		    else HoleZeichen := '$'
	    end (* HoleZeichen *);

	(* wie vorher, liest aber auch Leerzeichen *)

	function NaechstesZeichen: char;
	    begin
		if Position <= Length(a)
		    then begin
			NaechstesZeichen := a[Position];
			Inc(Position) end
		    else NaechstesZeichen := '$'
	    end (* NaechstesZeichen *);

	(* liest kompletten Bezeichner, bis erstes nicht in Buchstabe
	   enthaltene Zeichen auftritt. Bem. 1 *)

	procedure LeseName(var Name: string);
	    begin
		Name := '';
		repeat
		    Name := Name + UpCase(ch);
		    ch := NaechstesZeichen
		until not (ch in Buchstabe)
	    end (* LeseName *);

(*------------------------------------------------------------------------*)

	function Power(a, b: real): real;
	    begin
	    end;

       (* Parsen einer Zahl. Bem1. *)

	function Zahl: real;
	    var h, d : real;
		e : integer;
		Negativ : boolean;
	    begin
		h := 0;
		repeat
		    h := h * 10 + (Ord(ch) - Ord('0'));
		    ch := NaechstesZeichen
		until not (ch in ['0'..'9']);
		if ch = '.' then begin
		    ch := Naechsteszeichen;
		    d := 0.1;
		    while ch in ['0'..'9'] do begin
			h := h + d * (Ord(ch) - Ord('0'));
			d := d / 10;
			ch := NaechstesZeichen end;
		    if (ch = 'e') or (ch = 'E') then begin
			ch := NaechstesZeichen;
			Negativ := false;
			if ch = '+'
			    then ch := NaechstesZeichen
			    else if ch = '-'
				then begin
				    Negativ := true;
				    ch := NaechstesZeichen end;
			e := 0;
			if not (ch in ['0'..'9'])
			    then SetError(ESyntax)
			    else begin
				repeat
				    e := e * 10 + Ord(ch) - Ord('0');
				    ch := NaechstesZeichen
				until not (ch in ['0'..'9']);
				if Negativ then e := -e;
				h := h * Power(10, e) end end end;
		Zahl := h
	    end (* Zahl *);

(*------------------------------------------------------------------------*)

	function Ausdruck: real; forward;
	function Term: real; forward;
	function Faktor: real; forward;
	function Exponent: real; forward;

	(* prÅft 'Name' auf öbereinstimmung mit definierten Funktionen und
	   setzt 'Erfolg' entsprechend. RÅckgabe der Auswertung in 'Ergebnis'.
	   Bem. 1, dabei enthÑlt 'ch' die îffnende Klammer *)

	procedure PruefeFunktion(Name: string; var Erfolg: boolean; var Ergebnis: real);
	    type f = (sinus, cosinus, wurzel, arctang, fehler);
	    const n : array[f] of string = ('SIN', 'COS', 'SQR', 'ATN', '');
	    var i : f;
	    begin
		i := sinus;
		while (i < fehler) and (Name <> n[i]) do Inc(i);
		if i = fehler
		    then Erfolg := false
		    else begin
			Erfolg := true;
			if ch <> '('
			    then SetError(ESyntax)
			    else begin
				ch := HoleZeichen;
				case i of
				    sinus   : Ergebnis := Sin(Ausdruck);
				    cosinus : Ergebnis := Cos(Ausdruck);
				    wurzel  : Ergebnis := Sqrt(Ausdruck);
				    arctang : Ergebnis := ArcTan(Ausdruck) end;
				if ch <> ')'
				    then SetError(ESyntax)
				    else ch := HoleZeichen end end
	    end (* PruefeFunktion *);

	(* Parsen eines Ausdruck, Term, Faktor, Exponenten gemÑ·
	   Syntaxdiagrammen. FÅr alle Routinen gilt Bem. 1 *)

	function Ausdruck: real;
	    var h : real;
		Neg : boolean;
	    begin
		Neg := false;
		if ch = '+'
		    then ch := HoleZeichen
		    else if ch = '-' then begin
				Neg := true;
				ch := HoleZeichen end;
		h := Term;
		while ((ch = '+') or (ch = '-')) and (FehlerZustand = ENoError) do
		    if ch = '+'
			then begin
			    ch := HoleZeichen;
			    h := h + Term end
			else begin
			    ch := HoleZeichen;
			    h := h - Term end;
		if Neg
		    then Ausdruck := -h
		    else Ausdruck := h
	    end (* Ausdruck *);

	function Term: real;
	    var h : real;
	    begin
		h := Faktor;
		while ((ch = '*') or (ch = '/')) and (FehlerZustand = ENoError) do
		    if ch = '*'
			then begin
			    ch := HoleZeichen;
			    h := h * Term end
			else begin
			    ch := HoleZeichen;
			    h := h / Term end;
		Term := h
	    end (* Term *);

	function Faktor: real;
	    var h : real;
	    begin
		h := Exponent;
		while (ch = '^') and (FehlerZustand = ENoError) do begin
		    ch := HoleZeichen;
		    h := Power(h, Exponent) end;
		Faktor := h
	    end (* Faktor *);

	function Exponent: real;
	    var Name : string;
		Erfolg : boolean;
		Ergebnis : real;
	    begin
		case ch of
		    '('		: begin
					ch := HoleZeichen;
					Exponent := Ausdruck;
					if ch = ')'
					    then ch := HoleZeichen
					    else SetError(ESyntax)
				  end;
		    'A'..'Z',
		      'a'..'z' : begin
					LeseName(Name);
					PruefeFunktion(Name, Erfolg, Ergebnis);
					if not Erfolg
					    then VarZugriff(Name, Erfolg, Ergebnis);
					if not Erfolg
					    then SetError(EVar)
					    else Exponent := Ergebnis
				 end;
		    '0'..'9'   : Exponent := Zahl
		    else	 begin
					SetError(ESyntax);
					ch := HoleZeichen
				 end end
	    end (* Exponent *);

	begin
		(* Initialisierung der globalen Variablen von 'Berechne' *)
	    Position := 1;
	    FehlerZustand := ENoError;
	    ch := HoleZeichen;
		(* Auswertung *)
	    Erg := Ausdruck;
		(* PrÅfen, ob Ende des Ausdruck erreicht *)
	    if HoleZeichen <> '$' then SetError(ESyntax);
		(* RÅckgabe des Fehlercode *)
	    Fehler := FehlerZustand
	end (* Berechne *);

    (* Dummyprozedur fÅr Variablenzugriffe, die nur Fehlerflag setzt *)

    procedure DummyVarProc(a: string; var b: boolean; var c: real);
	begin
	    b := false
	end (* DummyVarProc *);

    end (* Rechner *).