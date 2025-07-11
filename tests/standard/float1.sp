program single1;

procedure print (x: single);
    begin
	write (x:10:5)
    end;

procedure proc6 (p1, p2, p3, p4, p5, p6: single);
    begin
	print (p1); print (p2); print (p3); print (p4); print (p5); 
	print (p6)
    end;

procedure proc7 (p1, p2, p3, p4, p5, p6, p7: single);
    begin
	print (p1); print (p2); print (p3); print (p4); print (p5); 
	print (p6); print (p7)
    end;

procedure proc8 (p1, p2, p3, p4, p5, p6, p7, p8: single);
    begin
	print (p1); print (p2); print (p3); print (p4); print (p5);
	print (p6); print (p7); print (p8)
    end;

procedure proc9 (p1, p2, p3, p4, p5, p6, p7, p8, p9: single);
    begin
	print (p1); print (p2); print (p3); print (p4); print (p5);
	print (p6); print (p7); print (p8); print (p9)
    end;

procedure proc10 (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10: single);
    begin
	print (p1); print (p2); print (p3); print (p4); print (p5);
	print (p6); print (p7); print (p8); print (p9); print (p10)
    end;

procedure test;
    begin
	proc6 (1, 2, 3, 4, 5, 6); writeln;
	proc7 (1, 2, 3, 4, 5, 6, 7); writeln;
	proc8 (1, 2, 3, 4, 5, 6, 7, 8); writeln;
	proc9 (1, 2, 3, 4, 5, 6, 7, 8, 9); writeln;
	proc10 (1, 2, 3, 4, 5, 6, 7, 8, 9, 10); writeln;
    end;

begin
    test
end.
