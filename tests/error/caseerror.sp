program caseerror;

var
    a: integer;

begin
    a := 5;
    case a of
        1, 2, 3: 
	    write (1);
        4..6:
            write (2);
        5..7:
	    write (3);
        3:
	    write (4)
    end
end.

	   
