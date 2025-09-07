program dumprom;

uses files;

const
    fn = 'DSK0.ROM.DAT';

var
    f: file;
    rom: byte absolute 0;

begin
    assign (f, fn);
    rewrite (f, 128);
    blockwrite (f, rom, 8192 div 128);
    close (f);
    writeln ('Console ROM dumped to file ', fn);
    waitkey
end.