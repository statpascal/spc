program casetest;

var
    i: integer;

begin
    // jump table with else branch    
    for i := -3 to 12 do
        begin
            gotoxy (0, i + 3);
            write ('i=', i:2, ': ');
            case i of
                -2:
                    write ('-2');
                1..3:
                    write ('1..3');
                5: 
                    write ('5');
                7..8:
                    write ('7..8');
                else
                    write ('else')
            end
        end;
        
    // sequence of comparisons with else branch
    for i := -3 to 12 do
        begin
            gotoxy (12, i + 3);
            case i of
                -2:
                    write ('-2');
                1..3:
                    write ('1..3');
                5: 
                    write ('5');
                7..8:
                    write ('7..8');
                1000:   // value disables jump table 
                    write ('1000');
                else
                    write ('else')
            end
        end;
        
    // jump table without else branch
    for i := -3 to 12 do
        begin
            gotoxy (18, i + 3);
            case i of
                -2:
                    write ('-2');
                1..3:
                    write ('1..3');
                5: 
                    write ('5');
                7..8:
                    write ('7..8')
            end
        end;
        
    // sequence of comparisons without else branch
    for i := -3 to 12 do
        begin
            gotoxy (24, i + 3);
            case i of
                -2:
                    write ('-2');
                1..3:
                    write ('1..3');
                5: 
                    write ('5');
                7..8:
                    write ('7..8');
                1000:   // value disables jump table 
                    write ('1000')
            end
        end;
    waitkey
end.
