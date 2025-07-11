program supermat;

(* Dummy routines not present in StatPascal *)

procedure sound (a: integer);
begin
end;

procedure nosound;
begin
end;

procedure delay (a: integer);
begin
end;

procedure clrscr;
begin
end;

procedure randomize;
begin
end;

%%%% Originalprogramm

label vorne,weiter,ende,unten,stop,
      rech1,rech2,rech3,rech4,rech5,rech6,rech7,rech8,
      noch1,noch2,noch3,noch4,noch5,noch6,noch7,noch8,
      richtig,richtig1,richtig2,richtig3,richtig4,richtig5,
      richtig6,richtig7,richtig8,
      falsch1,falsch2,falsch3,falsch4,falsch5,falsch6,falsch7,falsch8;
var   ton,graphmode,graphdriver,
      j,grad,stufe,m,n,summe,prod,diff,qot,og,k,i,w,neg,fehl,zu:integer;
           name:string;
begin clrscr;writeln;writeln;writeln;delay(1000);
 sound(200);delay(200);nosound;delay(200);sound(200);delay(200);
         sound(300);delay(300);sound(200);delay(200);sound(400);
         delay(200);nosound;delay(400);

writeln('          Guten Tag! ');delay(1000);
      sound(300);delay(90);sound(304);delay(10);
      sound(311);delay(10);sound(318);delay(10);
      sound(328);delay(20);sound(330);delay(10);sound(333);delay(10);
      sound(339);delay(10);sound(349);delay(10);
      sound(361);delay(5);sound(368);delay(5);sound(374);delay(5);
      sound(383);delay(5);sound(388);delay(5);sound(396);delay(5);
      sound(403);delay(5);sound(411);delay(5);sound(419);delay(5);
      sound(428);delay(5);sound(436);delay(5);sound(445);delay(5);
      sound(454);delay(5);sound(464);delay(5);sound(473);delay(5);
      nosound;delay(100);

      sound(400);delay(40);sound(390);delay(40);
      sound(385);delay(40);sound(380);delay(40);nosound;delay(300);

      sound(366);delay(20);
      sound(370);delay(20);sound(374);delay(20);sound(378);delay(20);
      sound(384);delay(20);sound(378);delay(10);sound(372);delay(10);
      sound(366);delay(20);sound(372);delay(10);sound(377);delay(10);
      sound(380);delay(40);sound(390);delay(40);
      sound(400);delay(40);sound(410);delay(30);sound(405);delay(20);
      nosound;delay(400);
writeln;writeln;writeln;delay(800);
writeln('                            Willkommen zu Supermat!');
      sound(100);delay(500);sound(200);delay(500);
      sound(300);delay(500);sound(400);delay(500);
      sound(500);delay(500);sound(600);delay(500);
      sound(600);delay(500);sound(500);delay(500);
      sound(400);delay(500);sound(300);delay(500);
      sound(600);delay(1000);sound(300);delay(500);
      sound(400);delay(500);sound(300);delay(500);
      sound(200);delay(500);sound(500);delay(2000);nosound;
(*
      graphdriver:=detect;
                initgraph(graphdriver,graphmode,'\pcturbo');
                setbkcolor(black);setcolor(white);
                moveto(100,100);lineto(400,100);lineto(500,200);
                lineto(100,100);
                setfillstyle(1,white);floodfill(150,105,white);
                delay(2000);
                closegraph;clrscr;writeln;writeln;writeln;
*)
      writeln('Bitte Deinen Namen eingeben');writeln;
      read(name);writeln;clrscr;
      randomize;
      write('Guten Tag ');write(name);writeln (' !  ');writeln;writeln;
      writeln('Bitte den Schwierigkeitgrad 1 - 10 wählen!');writeln;
      write('Schwierigkeitsgrad:  ');read(grad);writeln;
      stufe:=1;og:=3;
vorne:neg:=0;writeln;writeln;writeln('            Stufe  ',stufe);
for i:=1 to 3 do begin;fehl:=0;
        j:=random(8)+1;
        case j of
        1:goto rech1;
        2:goto rech2;
        3:goto rech3;
        4:goto rech4;
        5:goto rech5;
        6:goto rech6;
        7:goto rech7;
        8:goto rech8;
end;

rech1:writeln;
                         m:=random(og);n:=random(og);summe:=m+n;
                         noch1:write(m,' + ',n,' = ');read(k);
                         case k=summe of
                              true:goto richtig1;
                                   false:goto falsch1;end;
richtig1:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch1:
        sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(100);delay(200);
        sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch1;

rech2:writeln;
                         m:=random(og);n:=random(og);prod:=m*n;
                         noch2:write(m,' * ',n,' = ');read(k);
                         case k=prod of
                              true:goto richtig2;
                                   false:goto falsch2;end;
richtig2:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch2: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch2;

rech3:writeln;
                         m:=random(og);diff:=random(og);n:=m+diff;
                         noch3:write(n,' - ',m,' = ');read(k);
                         case k=diff of
                              true:goto richtig3;
                                   false:goto falsch3;end;
richtig3:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch3: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch3;

rech4:writeln;
                         m:=random(og)+1;qot:=random(og)+1;n:=m*qot;
                         noch4:write(n,' : ',m,' = ');read(k);
                         case k=qot of
                              true:goto richtig4;
                                   false:goto falsch4;end;
richtig4:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch4: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch4;

rech5:writeln;
                         m:=random(og);n:=random(og);zu:=random(og);
                         summe:=m*n+zu;
           noch5:write(m,' * ',n,' + ',zu,' = ');read(k);
                         case k=summe of
                              true:goto richtig5;
                                   false:goto falsch5;end;
richtig5:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch5: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch5;

rech6:writeln;
                         m:=random(og);n:=random(og);zu:=random(og);
                         prod:=m*n*zu;
                         noch6:write(m,' * ',n,' * ',zu,' = ');read(k);
                         case k=prod of
                              true:goto richtig6;
                                   false:goto falsch6;end;
richtig6: writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch6: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
         writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch6;

rech7:writeln;
                         m:=random(og)+1;diff:=random(og);zu:=random(og)+1;
                         m:=m+diff;n:=m*zu-diff;
                         noch7:write(m,' * ',zu,' - ',diff,' = ');read(k);
                         case k=n of
                              true:goto richtig7;
                                   false:goto falsch7;end;
richtig7:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch7: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch7;

rech8:writeln;
                         m:=random(og)+1;qot:=random(og)+1;zu:=random(og);
                         n:=m*qot;summe:=qot+zu;
                         noch8:write(n,' : ',m,' + ',zu,' = ');read(k);
                         case k=summe of
                              true:goto richtig8;
                                   false:goto falsch8;end;
richtig8:writeln;write('     Gut gemacht  ',name,'  !  ');writeln;writeln;
        goto richtig;
falsch8: sound(500);delay(200);sound(450);delay(200);sound(350);
        delay(200);sound(200);delay(200);sound(70);delay(300);nosound;
        writeln;write('     Leider falsch ',name,' !  ');writeln;
               neg:=1;fehl:=fehl+1;
        if fehl<3 then goto noch8;
richtig: sound(200);delay(200);nosound;delay(200);sound(200);delay(200);
         sound(300);delay(300);sound(200);delay(200);sound(400);
         delay(200);nosound;
        goto weiter;
weiter:end;
      if neg = 1 then goto unten;writeln;
 writeln('Du hast immer richtig gerechnet. Du kommst in die nächste Stufe!');writeln;
      stufe:=stufe+1;og:=og+grad;goto stop;
unten:writeln('Du mußt nochmal in derselben Stufe rechnen!');writeln;writeln;
stop:write('Weiter (nein/ja) 0/1 ? ');read(w);
      case w>0 of
          true:goto vorne;
          false:goto ende;end;
ende:writeln;writeln;end.
