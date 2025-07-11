unit AVL;

interface

(* AVL - BÑume *)

type Richtung = (links, gerade, rechts);

     AVLBaum  = ^AVLNode;
     AVLNode  = record
		  L, R : AVLBaum;
		  Schiefe : Richtung;
		  DataPtr, KeyPtr : pointer;
		  DataSize, KeySize : word
     end;

     (* AVLNode wird exportiert, um Traversierung zu ermîglichen *)

     VglFkt   = function(a, b: pointer): integer;

     (* Funktionstyp fÅr Vergleich der SchlÅsselZeiger. Mu· folgende Ergebnisse
	liefern:
	a < b : < 0; a = b : = 0; a > b : > 0
     *)

procedure FuegeEin(var p: AVLBaum; Daten, Schluessel: pointer;
		   Datengroesse, Schluesselgroesse: word; Vgl: VglFkt);

(* EinfÅgen in AVL-Baum. Von Daten und SchlÅssel adressierte Speicherbereiche
   werden kopiert, Vgl mu· obiger Definition entsprechen *)

procedure Loesche(var p: AVLBaum);

(* Lîschen aller EintrÑge *)

function  Suche(p: AVLBaum; Schluessel: pointer; Vgl: VglFkt): pointer;

(* liefert Zeiger auf zu SchlÅssel gehîrenden Dateneintrag zurÅck *)


implementation

var  Terminiere : boolean;
     AltRichtung : Richtung;

     DatenGr, SchluesselGr : word;		(* Zwischenspeichern der Parameter *)
     Vergleich : VglFkt;
     DatenZgr, SchluesselZgr : pointer;

     Dummy : integer;

(* Routinen zur Balancierung des Baumes *)

(* Baum in p linkslastig, einfache Rotation *)

procedure RotiereR(var p: AVLBAUM);
    var q : AVLBaum;
    begin
	q := p;
	p := p^.L;
	q^.L := p^.R;
	p^.R := q;
	p^.Schiefe := gerade;
	p^.R^.Schiefe := gerade
    end;

(* Baum in p rechtslastig, einfache Rotation *)

procedure RotiereL(var p: AVLBaum);
    var q : AVLBaum;
    begin
	q := p;
	p := p^.R;
	q^.R := p^.L;
	p^.L := q;
	p^.Schiefe := gerade;
	p^.L^.Schiefe := gerade
    end;

(* Baum in p linkslastig, doppelte Rotation *)

procedure RotiereRR(var p: AVLBaum);
    var q : AVLBaum;
    begin
	q := p;
	p := p^.L^.R;
	q^.L^.R := p^.L;
	p^.L := q^.L;
	q^.L := p^.R;
	p^.R := q;
	if p^.Schiefe = rechts then begin
	    p^.R^.Schiefe := gerade; p^.L^.Schiefe := links end
	else if p^.Schiefe = links then begin
	    p^.L^.Schiefe := gerade; p^.R^.Schiefe := rechts end
	else begin
	    p^.L^.Schiefe := gerade; p^.R^.Schiefe := gerade
	end;
	p^.Schiefe := gerade
    end;

(* Baum in p rechtslastig, doppelte Rotation *)

procedure RotiereLL(var p: AVLBaum);
    var q : AVLBaum;
    begin
	q := p;
	p := p^.R^.L;
	q^.R^.L := p^.R;
	p^.R := q^.R;
	q^.R := p^.L;
	p^.L := q;
	if p^.Schiefe = rechts then begin
	    p^.L^.Schiefe := links; p^.R^.Schiefe := gerade end
	else if p^.Schiefe = links then begin
	    p^.L^.Schiefe := gerade; p^.R^.Schiefe := rechts end
	else begin
	    p^.L^.Schiefe := gerade; p^.R^.Schiefe := gerade
	end;
	p^.Schiefe := gerade
    end;

(* EinfÅgen der gesetzten globalen Zeiger in Baum p *)

procedure Einfuegen(var p: AVLBaum);
    var NeuRichtung : Richtung;
    begin
	if p = nil then begin
	    New(p); p^.L := nil; p^.R := nil;
	    p^.Schiefe := gerade;
	    if (DatenZgr <> nil) and (DatenGr <> 0) then begin
		GetMem(p^.DataPtr, DatenGr);
		Move(DatenZgr^, p^.DataPtr^, DatenGr);
		p^.DataSize := DatenGr end
	    else p^.Dataptr := nil;
	    GetMem(p^.KeyPtr, SchluesselGr);
	    p^.KeySize := SchluesselGr;
	    Move(SchluesselZgr^, p^.KeyPtr^, SchluesselGr) end
	else begin
	    dummy := Vergleich(p^.KeyPtr, SchluesselZgr);
	    if dummy < 0 then begin
		Einfuegen(p^.R); NeuRichtung := rechts end
	    else if dummy > 0 then begin
		Einfuegen(p^.L); NeuRichtung := links end
	    else begin	(* Eintrag ersetzen *)
		if p^.DataPtr <> nil then FreeMem(p^.DataPtr, p^.DataSize);
		if (DatenZgr <> nil) and (DatenGr <> 0) then begin
		    GetMem(p^.DataPtr, DatenGr);
		    Move(DatenZgr^, p^.DataPtr^, DatenGr);
		    p^.DataSize := DatenGr end
		else p^.Dataptr := nil;
		Terminiere := true	(* keine Rebalancierung nîtig *)
	    end;
            if not Terminiere then
		if p^.Schiefe = NeuRichtung then begin 	(* Rotation nîtig *)
		    if NeuRichtung <> AltRichtung then 	(* doppelt *)
			if NeuRichtung = links then
			    RotiereRR(p)
			else
			    RotiereLL(p)
		    else				(* einfach *)
			if NeuRichtung = links then
			    RotiereR(p)
			else
			    RotiereL(p);
		    Terminiere := true end
		else if p^.Schiefe = gerade then begin
		    p^.Schiefe := NeuRichtung;
		    AltRichtung := NeuRichtung end
		else begin
		    p^.Schiefe := gerade;
		    Terminiere := true
		end
	end
    end;

(* Implementierung der Schnittstelle und Setzen der globalen Variablen *)

procedure FuegeEin(var p: AVLBaum; Daten, Schluessel: pointer;
		   Datengroesse, Schluesselgroesse: word; Vgl: VglFkt);
    begin
	Terminiere := false;
	DatenGr := Datengroesse;
	SchluesselGr := Schluesselgroesse;
	Vergleich := Vgl;
	DatenZgr := Daten;
	SchluesselZgr := Schluessel;
	Einfuegen(p)
    end;

(************************************************************************)

(* Lîschen des Baumes *)

procedure LoescheBaum(p: AVLBaum);
    begin
	if p <> nil then begin
	    LoescheBaum(p^.L); LoescheBaum(p^.R);
	    if p^.DataPtr <> nil then FreeMem(p^.Dataptr, p^.Datasize);
	    freemem(p^.keyptr, p^.keysize)
	end
    end;

procedure Loesche(var p: AVLBaum);
    begin
	LoescheBaum(p);
	p := nil
    end;

(* Absuchen des Baumes *)

procedure Search(p: AVLBaum);
    begin
	dummy := vergleich(p^.keyptr, schluesselzgr);
	if dummy < 0 then search(p^.r)
	else if dummy > 0 then search(p^.l)
	else datenZgr := p^.dataptr
    end;

function  Suche(p: AVLBaum; Schluessel: pointer; Vgl: VglFkt): pointer;
    begin
	vergleich := vgl;
	schluesselzgr := schluessel;
	search(p);
	suche := datenzgr
    end;

end.
