program textfile;

// File output will be merged with system.pas when it is stable 
uses files;


procedure writeData (var f: text);

    var
        i: integer;
        ch: char;
        b: boolean;

    begin
        for i := 1 to 10 do
            writeln (f, 'Hello, world!', i:5);
            
        for ch := 'A' to 'Z' do
            write (f, ch:2);
        writeln (f);    
        
        for b := false to true do
            write (f, b:10);
        writeln (f)
    end;

var
    f: text;
    
begin
    assign (f, 'DSK0.TEST.TXT');
    rewrite (f);
    writeData (f);
    close (f);
    
    writeln ('File written - now writing to screen');
    writeData (output);

    waitkey
end.
