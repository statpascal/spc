program casetest;

type
    colors = (red, green, blue, yellow, black);
    subcolors = green..yellow;

var 
    i: integer;
    c: colors;
    ch: char;

function f: integer;
    begin
	f := 1
    end;

function g: subcolors;
    begin
	g := yellow
    end;

begin
    case f of
	1: 
	    writeln ('OK')
        else
	    writeln ('Error')
    end;

    case g of
	yellow: 
	    writeln ('OK')
    	else
	    writeln ('Error')
    end;

    for c := red to yellow do
        case c of 
	    red, green:
		case c of
  		    red: 
			writeln ('red');
        	    green: 
			writeln ('green')
		    else
		    	writeln ('Error')
		end;
            blue: 
		writeln ('blue');
            else 
            	writeln ('Other color');
        end;

    i := 5;
    case i + 4 of
	1: 
	    writeln ('This is wrong');
        2..4, 100..105, 200..300:
 	    writeln ('Also wrong');
        -7..-3, 8 + 1, 17: 
	    writeln ('OK');
    end;

    for ch := 'E' downto 'A' do
	case ch of
	    'B': 
		writeln ('B');
	    'A', 'C': 
		writeln ('A or C');
	    'D'..'E': 
		writeln ('D or E')
        end;

    case pred (i) = 4 of
	false: 
	    writeln ('false');
	true: 
	    writeln ('true')
    end;
    case succ (i) = 5 of
        false..true: 
	    writeln ('OK')
    	else
	    writeln ('Wrong')
    end;

    for i := -5 to 5 do
        case i of
	    -3, -1:
		writeln ('-3 or -1');
	    1, 3:
		writeln ('1 or 3')
	    else
		writeln (i)
	end;

    for i := -5 to 5 do
        case i of
	    -3, -1:
		writeln (i, ' is negative');
	    1, 3:
		writeln (i, ' is positive')
	end;

    for i := 5 to 20 do
	case i of
	    8..10:
		writeln (i, ' 8..10');
	    12..15, 18:
		writeln (i, ' 12..15, 18')
	end;

    for i := 5 to 20 do
	case i of
	    -5, 25:
		writeln (i, ' ????')
	end;

    for i := 5 to 20 do
	case i of
	    2:
	end;

    for i := -5 to 5 do
	case 1000 * i of
	   -5000: 
		writeln (i:5, ' -5000');
	   -4000, -3000:
		writeln (i:5, ' -4000, -3000');
	   2000..4000:
		writeln (i:5, ' 2000..4000');
	   else
		writeln (i:5, ' other')
        end

end.
