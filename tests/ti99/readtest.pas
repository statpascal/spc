program readtest;

function read_line__console: string;
    const
        FctnS = #136;
        Enter = #13;
    var
        row, col, pos, count: integer;
        ch: char;
    begin
        col := whereX;
        row := whereY;
        result [0] := #0;
        pos := 1;
        count := 0;
        repeat
            ch := getKey;
            case ch of
                FctnS:
                    if pos > 1 then
                        begin
                            dec (pos);
                            dec (result [0]);
                            gotoxy (col, row);
                            write (result, ' ');
                            if col + pos < 31 then write (' ')
                        end;
                #32..#127:
                    begin
                        result [pos] := ch;
                        if col + pos < 31 then
                            begin
                                inc (pos);
                                inc (result [0])
                            end;
                        gotoxy (col, row);
                        write (result)
                    end
            end;
            inc (count);
            if count and $ff = 0 then
                begin
                    gotoxy (col + pos - 1, row);
                    if odd (count shr 8) then write (#$1f) else write (' ')
                end;
        until ch = Enter;
        writeln;
    end;

var
    s: string;
    
begin
    write ('Input: ');
    readln (s);
    writeln ('Got:   ', s);
    while keypressed do;
    waitkey
end.