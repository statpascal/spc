program rectest4;

type 
   rec = record 
	a: char;
	b: int64;
	c: boolean;
	d: record
	    d1: char;
	    d2: int64;
	    d3: record
		d11: char;
		d22: int64;
		d33: boolean
	    end
	end
    end;

procedure setR (var r: rec);
    begin
	r.d.d3.d22 := 5;
	r.d.d1 := 'A';
	r.d.d3.d33 := false
    end;

procedure print (r: rec);
    begin
        writeln (r.d.d3.d22, ' ', r.d.d1);
        writeln (sizeof (rec), ' ', addr (r.d.d3.d22) - addr (r), ' ', addr (r.d.d1) - addr (r))
    end;

procedure test;
    var
	r: rec;
    begin
	setR (r);
	print (r)
    end;

begin 
    test
end.
