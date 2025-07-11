unit Tex; (* 04.10.90 *)

(* Prozeduren zur Erzeugung einer TeX-Datei *)

interface

type RealFunc = function (x: real): real;

procedure TeXBeginPic (x0, y0, x1, y1, ux, uy: real);
  (* Startbefehl fÅr Picture-Umgebung und Definition eines Koordinatensystems;
     ux und uy definieren LÑnge einer Einheit auf x- bzw. y-Achse in cm *)

procedure TeXEndPic;
  (* Ende-Befehl fÅr Picture-Umgebung erzeugen *)

procedure TeXSetResolution (x, y: integer);
  (* Punkte je Einheit bei Plotten einer Linie *)

procedure TeXOpen (s: string);
  (* ôffnet TeX-Ausgabedatei unter angegebenem Namen *)

procedure TeXClose;
  (* Schlie·en der TeX-Ausgabedatei *)

procedure TexFillCircleCmd (x, y, r: real);
  (* ausgefÅllter Kreis. Radius in Einheiten auf x-Achse *)

procedure TeXCircleCmd (x, y, r: real);
  (* normaler Kreis *)

procedure TeXArc (x, y, r: real; phi0, phi1: real; n: integer);
  (* Kreissegment mit n StÅtzpunkten *)

procedure TeXCircle (x, y, r: real; n: integer);
  (* Plotten eines Kreises mit n StÅtzpunkten *)

procedure TeXLine (x0, y0, x1, y1: real);
  (* bildet Linie aus Einzelpunkten (jeder Steigungswinkel mîglich.
     Die Linie wird nur im Bereich des Koordinatensystems gezeichnet. *)

procedure TeXLineCmd0 (x0, y0: real; dx, dy: integer; Len: real);
  (* "wîrtliche" Ausgabe der Parameter *)

procedure TeXThinLines;
procedure TeXThickLines;
  (* Linienart fÅr \line-Kommando *)

procedure TeXLineCmd (x0, y0, x1, y1: real);
  (* zieht Linie unter Verwendung des \line-Kommandos. y1 wird verschoben,
     um zulÑssigen Steigungswinkel zu erreichen. Die Linie wird nur im Bereich
     des Koordinatensystems gezeichnet. *)

procedure TeXVector (x0, y0, x1, y1: real);
  (* wie TeXLineCmd, Vektorpfeil bei (x1, y1) *)

procedure TeXGetAngle (phi: real; var dx, dy: integer; LineCmd: boolean);
  (* liefert fÅr phi im Intervall (-Pi/2, Pi/2] beste mîgliche Steigung.
     Ist LineCmd false, werden die eingeschrÑnkten Steigungen fÅr
     \vector-Kommando berÅcksichtigt *)

procedure TeXFrame (x0, y0, dx, dy: real);
  (* zeichnet Rechteck mit linker unterer Ecke in (x0, y0) *)

procedure TeXDashFrame (x0, y0, dash, dx, dy: real);
  (* zeichnet gestricheltes Rechteck mit StrichelungslÑnge "dash" *)

procedure TeXAxis (x0, x1, dx, y0, y1, dy: real; numbersleft, numberslow: boolean);
  (* Ausgabe eines Achsenkreuzes von x0 bis x1 und y0 bis y1.
     dx und dy definieren AbstÑnde der Markierungen auf x- bzw. y-Achse,
     numbersleft und numberslow bestimmen Lage der ausgegebenen Zahlen
     (links bzw. unterhalb der Achse). *)

procedure TeXPutText (x, y: real; position: string; s: string);
  (* Ausgabe des String s bei (x, y). Position enthÑlt Positionierungsbefehl *)

procedure TeXPlot (f: RealFunc; a, b, y0, y1: real; n: integer);
  (* Plotten der Åbergebenen Funktion im Fenster a <= x <= b und
     y0 <= y <= y1. Anzahl der Ñqudistanten StÅtzstellen in n, diese
     werden durch Linien verbunden. *)

procedure Clip (var x0, y0, x1, y1: real; fx0, fy0, fx1, fy1: real; var Sichtbar: boolean);
  (* Clipping-Algorithmus von Cohen/Sutherland *)


implementation

var     TeXOut  : text;
        XMin, XMax, YMin, YMax : real;  (* gewÑhltes Koordinatensystems *)
        YScale : real;                  (* Dehnungsfaktor fÅr y-Achse *)
        XRes, YRes : real;              (* Punkte je Einheit bei Linien *)
        UnitX : real;                   (* LÑnge Einheit x-Richtung *)

function tan (x: real): real;
    begin
        tan := sin (x) / cos (x)
    end;

(* Punkt in Ausgabesystem umrechnen *)

procedure Adjust (var x, y: real);
    begin
        x := x - XMin;
        y := (y - YMin) * YScale
    end;


procedure TeXBeginPic (x0, y0, x1, y1, ux, uy: real);
    begin
        YScale := uy / ux;
        XRes := 200;            (* Defaultwerte fÅr Linien *)
        YRes := XRes / YScale;
        XMin := x0; YMin := y0;
        XMax := x1; YMax := y1;
        UnitX := ux;
        Writeln (TeXOut, '\unitlength', ux:9:6, 'cm');
        Writeln (TeXOut, '\begin{picture}(', XMax - XMin:9:6, ',', (YMax - YMin) * YScale:9:6, ')')
    end;

procedure TeXEndPic;
    begin
        Writeln (TeXOut, '\end{picture}')
    end;

procedure TeXSetResolution (x, y: integer);
    begin
        xres := x; yres := y / YScale
    end;

procedure texopen (s: string);
    begin
        assign (texout, s);
        rewrite (texout)
    end;

procedure texclose;
    begin
        close (texout)
    end;

procedure texfillcirclecmd (x, y, r: real);
    begin
        writeln (texout, '\put(', x - XMin:9:6, ',', (y - YMin) * YScale:9:6, '){\circle*{', r:9:6, '}}')
    end;

procedure TeXCircleCmd (x, y, r: real);
    begin
        Adjust (x, y);
        Writeln (texout, '\put(', x:9:6, ',', y:9:6, '){\circle{', r:9:6, '}}')
    end;

procedure TeXArc (x, y, r: real; phi0, phi1: real; n: integer);
    var i : integer;
	xalt, yalt, xneu, yneu, w : real;
    begin
        xalt := x + r * cos (phi0); yalt := y + r * sin (phi0);
        for i := 1 to n + 1 do begin
            w := (i * phi1 + (n + 1 - i) * phi0) / (n + 1);
            xneu := x + r * cos (w); yneu := y + r * sin (w);
            TeXLine (xalt, yalt, xneu, yneu);
            xalt := xneu;
            yalt := yneu
        end
    end;

procedure TeXCircle (x, y, r: real; n: integer);
    var i : integer;
    begin
        for i := 1 to n do
            TeXLine (x + r * cos (2 * Pi * (i - 1) / n), y + r * sin (2 * Pi * (i - 1) / n),
                     x + r * cos (2 * Pi * i / n), y + r * sin (2 * Pi * i / n))
    end;

procedure texline (x0, y0, x1, y1: real);
    var PTotal : integer;
        Sichtbar : boolean;

    function IMax (a, b: real): integer;
        var m : real;
        begin
            m := a;
            if b > m then m := b;
            if 1.0 > m then m := 1.0;
            IMax := Round (m)
        end;

    begin
        Clip (x0, y0, x1, y1, XMin, YMin, XMax, YMax, Sichtbar);
        if not Sichtbar then Exit;
        (* Transformation in Ausgabesystem *)
        Adjust (x0, y0);
        Adjust (x1, y1);
        (* Linie zeichnen *)
        pTotal := IMax (Abs (x1 - x0) * xres, abs (y1 - y0) * yres);
        x0 := x0 - (0.04 / UnitX);      (* Punkt wird nach rechts verschoben ausgegeben *)
        x1 := x1 - (0.04 / UnitX);
        writeln (texout, '\multiput (', x0:9:6, ',', y0:9:6, ')(', (x1 - x0) / ptotal:9:6,
                         ',', (y1 - y0) / ptotal:9:6, '){', ptotal, '}{.}')
    end;

procedure TeXThinLines;
    begin
        Writeln (TeXOut, '\thinlines')
    end;

procedure TeXThickLines;
    begin
        Writeln (TeXOut, '\thicklines')
    end;

(* In dieser Form fÅr GERADE.PAS geeignet. Routine Umstellen auf
    (x0, y0) -> (x1, y1) mit Umrechnung in def. System und passender
    Wahl des verfÅgbaren Steigungswinkel!! *)

procedure Texlinecmd0 (x0, y0: real; dx, dy: integer; Len: real);
   begin
        writeln (texout, '\put (', x0:9:6, ',', y0:9:6, '){\line(', dx, ',', dy, '){', len:9:6, '}}')
   end;

const MaxEntry = 97;

type AngleRec = record
                        dx, dy : integer;
                        phi    : real
                end;

var Tab : array [1..MaxEntry] of AngleRec;

procedure Texgetangle (phi: real; var dx, dy: integer; LineCmd: boolean);
    var Match, LastMatch: integer;
    begin
        Match := 1;
        LastMatch := 1;
        while (Match < MaxEntry) and (phi > Tab[Match].phi) do begin
            LastMatch := Match;
            repeat Inc (Match)
            until LineCmd or (Abs (Tab[Match].dx) <= 4) and (Abs (Tab[Match].dy) <= 4)
        end;
        if phi < (tab[LastMatch].phi + tab[Match].phi) / 2 then begin
            dx := tab[LastMatch].dx; dy := tab[LastMatch].dy end
        else begin
            dx := tab[Match].dx; dy := tab[Match].dy
        end
    end;

procedure InitTab;
     const Anglelist : array [1..23, 0..1] of integer =
        ( (1, 1), (1, 2), (1, 3), (1, 4), (1, 5), (1, 6),
          (2, 1), (2, 3), (2, 5),
          (3, 1), (3, 2), (3, 4), (3, 5),
          (4, 1), (4, 3), (4, 5),
          (5, 1), (5, 2), (5, 3), (5, 4), (5, 6),
          (6, 1), (6, 5) );
    var i, j, index : integer;
        h : AngleRec;
    begin
        with tab[1] do begin
            dx := 0; dy := 1; phi := Pi / 2
        end;
        with tab[2] do begin
            dx := 0; dy := -1; phi := -Pi / 2
        end;
        with tab[3] do begin
            dx := 1; dy := 0; phi := 0
        end;
        with tab[4] do begin
            dx := -1; dy := 0; Phi := Pi
        end;
        tab[97] := tab[1];
        with tab[97] do phi := phi + 2 * Pi;
        index := 5;
        for i := 1 to 23 do begin
            with tab[index] do begin
                dx := AngleList[i, 0];
                dy := AngleList[i, 1];
                phi := arctan (dy / dx)
            end;
            inc (index);
            with tab [index] do begin
                dx := AngleList[i, 0];
                dy := -AngleList[i, 1];
                phi := arctan (dy / dx)
            end;
            inc (index);
            with tab [index] do begin
                dx := -AngleList[i, 0];
                dy := AngleList[i, 1];
                phi := Pi + arctan (dy / dx)
            end;
            inc (index);
            with tab [index] do begin
                dx := -AngleList[i, 0];
                dy := -AngleList[i, 1];
                phi := Pi + arctan (dy / dx)
            end;
            inc (index)
        end;
        (* Tabelle nach aufsteigenden Winkeln sortieren *)
        for i := 1 to MaxEntry - 1 do begin
            index := i;
            for j := i + 1 to MaxEntry do
                if tab[j].phi < tab[index].phi then index := j;
            h := tab[i]; tab[i] := tab[index]; tab[index] := h
        end
    end;

(* Berechnung von Steigung und LÑnge einer Linie, fÅr
   \line- und \vector-Kommandos verwendet. *)

procedure Parameter (var x0, y0, x1, y1: real; var dx, dy: integer; var Len: real; var Sichtbar: boolean; LineCmd: boolean);
    var Phi, DeltaX : real;
        Getauscht : boolean;
        x0min, x1max, y0min, y1max : real;      (* Grenzen des Ausgabefensters in Ausgabesystem *)

    procedure Swap (var a, b: real);
        var c : real;
        begin
            c := a; a := b; b := c
        end;

    begin
        Clip (x0, y0, x1, y1, XMin, YMin, XMax, YMax, Sichtbar);
        if not Sichtbar or (x0 = x1) and (y0 = y1) then begin
            Sichtbar := false; (* fÅr Åbr. FÑlle *)
            Exit
        end;
        Adjust (x0, y0);
        Adjust (x1, y1);
        x0min := xmin; x1max := xmax;
        y0min := ymin; y1max := ymax;
        Adjust (x0min, y0min);
        Adjust (x1max, y1max);
        if x1 < x0 then begin
            Swap (x1, x0); Swap (y1, y0);
            Getauscht := true end
        else Getauscht := false;
        if x1 - x0 <= 0.001 * Abs (y1 - y0) then
            if y1 > y0 then Phi := Pi / 2
            else Phi := -Pi / 2
        else Phi := arctan ((y1 - y0) / (x1- x0));
        TexGetAngle (Phi, dx, dy, LineCmd);
        if dx = 0 then Len := Abs (y1 - y0)
        else if dy = 0 then Len := x1 - x0
        else begin
(* !           Ist Berechnung von Phi ÅberflÅssig? *)
            Phi := arctan (dy / dx);    (* bester mîglicher Winkel *)
            y1 := (x1 - x0) * tan (Phi) + y0;
            if (y1 > y1max) or (y1 < y0min) then begin  (* y1 in Ausgabefenster bringen *)
                if y1 > y1max then deltax := (y1 - y1max) / tan (Phi)
                else deltax := (y0min - y1) / tan (phi);
                x1 := x1 - deltax
            end;
            Len := x1 - x0
        end;
        if Getauscht then begin
	    dx := -dx; dy := -dy;
	    Swap (x1, x0); Swap (y1, y0)
        end
    end;

procedure TexLineCmd (x0, y0, x1, y1: real);
    var dx, dy : integer;
        Len : real;
        Sichtbar : boolean;
    begin
        Parameter (x0, y0, x1, y1, dx, dy, Len, Sichtbar, true);
        if Sichtbar and (Len <> 0) then TeXLineCmd0 (x0, y0, dx, dy, Len)
    end;

procedure TeXVector (x0, y0, x1, y1: real);
    var dx, dy : integer;
        Len : real;
        Sichtbar : boolean;
    begin
        Parameter (x0, y0, x1, y1, dx, dy, Len, Sichtbar, false);
        if Sichtbar then Writeln (TeXOut, '\put(', x0:9:6, ',', y0:9:6, '){\vector(',
                                          dx, ',', dy, '){', Len:9:6, '}}')
    end;

procedure Texframe (x0, y0, dx, dy: real);
    begin
        x0 := x0 - XMin;
        y0 := (y0 - YMin) * YScale;
        dy := dy * YScale;
        writeln (texout, '\put(', x0:9:6, ',', y0:9:6, '){\framebox(', dx:9:6, ',', dy:9:6, '){}}')
    end;

procedure TeXDashFrame (x0, y0, dash, dx, dy: real);
    begin
        x0 := x0 - XMin;
        y0 := (y0 - YMin) * YScale;
        dy := dy * YScale;
        writeln (texout, '\put(', x0:9:6, ',', y0:9:6, '){\dashbox{', dash:9:6, '}(', dx:9:6, ',', dy:9:6, '){}}')
    end;

procedure TeXPutText (x, y: real; position: string; s: string);
    begin
        x := x - XMin;
        y := (y - YMin) * YScale;
        Writeln (TexOut, '\put (', x:9:6, ',', y:9:6, '){\makebox(0,0)[', position,
                         ']{', s, '}}')
    end;

procedure TeXAxis (x0, x1, dx, y0, y1, dy: real; numbersleft, numberslow: boolean);
    var i : integer;
        ds : real;              (* Strichbreite fÅr Achsen *)

    procedure XTick (x, ds: real; numberslow: boolean);
        var xstring: string;
        begin
            TeXLineCmd (x, -ds / YScale, x, ds / YScale);       (* Dehnung Striche verhindern *)
            Str (x:3:1, xstring);
            if numberslow then TeXPutText (x, -ds / YScale, 'tl', '\scriptsize ' + xstring)
            else TeXPutText (x, ds / YScale, 'bl', '\scriptsize ' + xstring)
        end;

    procedure YTick (y, ds: real; numbersleft: boolean);
        var xstring: string;
        begin
            TeXLineCmd (-ds, y, ds, y);
            str (y:3:1, xstring);
            if numbersleft then TeXPutText (-ds, y, 'br', '\scriptsize ' + xstring)
            else texputtext (ds, y, 'bl', '\scriptsize ' + xstring)
        end;

    begin
        Writeln (TeXout, '\put (', (x0 - XMin):9:6, ',', -YMin * YScale:9:6, '){\vector (1,0){', x1 - x0:9:6, '}}');
        Writeln (TeXout, '\put (', -XMin:9:6, ',', (y0-Ymin)*YScale:9:6, '){\vector (0,1){', (y1 - y0)*YScale:9:6, '}}');
        ds := 0.1 / UnitX;
        i := 1;
        while i * dx < x1 do begin
            XTick (i * dx, ds, numberslow); Inc (i)
        end;
        i := -1;
        while i * dx > x0 do begin
            XTick (i * dx, ds, numberslow); Dec (i)
        end;
        i := 1;
        while i * dy < y1 do begin
            YTick (i * dy, ds, numbersleft); Inc (i)
        end;
        i := -1;
        while i * dy > y0 do begin
            YTick (i * dy, ds, numbersleft); Dec (i)
        end
    end;


(* Clipping-Algorithmus von Cohen/Sutherland *)

procedure Clip (var x0,y0,x1,y1: real; fx0, fy0, fx1, fy1: real; var sichtbar: boolean);

    type
         Richtung = (links, rechts, oben, unten);
         Lage = set of Richtung;

    var
         L, L0, L1: Lage;
         x, y: real;

    procedure PunktLage (x, y: real; fx0, fy0, fx1, fy1: real; var L: Lage);
        begin
            L := [];
            if x < fx0 then L := [links]
            else if x > fx1 then L := [rechts];
            if y < fy0 then L := L + [unten]
            else if y > fy1 then L := L + [oben]
        end;

    begin
        PunktLage (x0, y0, fx0, fy0, fx1, fy1, L0);
        PunktLage (x1, y1, fx0, fy0, fx1, fy1, L1);
        if L0 * L1 <> [] then sichtbar := false
        else if (L0 = []) and (L1 = []) then sichtbar := true
        else begin
            L := L0; if L = [] then L := L1;
            if links in L then begin (* Schnittpunkt mit linker Kante *)
                y := y0 + (y1 - y0) * (fx0 - x0) / (x1 - x0);
                x := fx0 end
            else if rechts in L then begin (* Schnittpunkt mit rechter Kante *)
                y := y0 + (y1 - y0) * (fx1 - x0) / (x1 - x0);
                x := fx1 end
            else if unten in L then begin (* Schnittpunkt mit unterer Kante *)
                x := x0 + (x1 - x0) * (fy0 - y0) / (y1 - y0);
                y := fy0 end
            else if oben in L then begin (* Schnittpunkt mit oberer Kante *)
                x := x0 + (x1 - x0) * (fy1 - y0) / (y1 - y0);
                y := fy1 end;
            if L = L0 then begin x0 := x; y0 := y end
            else begin x1 := x; y1 := y end;
            Clip (x0,y0,x1,y1, fx0, fy0, fx1, fy1, sichtbar) end
    end;

procedure TeXPlot (f: realfunc; a, b, y0, y1: real; n: integer);
    var i : integer;
        x, y, xs, ys : real;
    begin
        xs := a;
        ys := f(a);
        for i := 1 to n do begin
            x := a + (b - a) / n * i;
            y := f(x);
            TeXLine (xs, ys, x, y);
            xs := x;
            ys := y
        end
    end;

begin
    InitTab
end.
