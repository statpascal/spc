program hextext;

begin
    writeln ($ff);
    writeln (-$ff);
    writeln ($ffffffff);
    writeln ($ff00 or $00ff);
    writeln ($ffff and not $ff);
    writeln (not (-1))
end.
