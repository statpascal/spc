program condtest;

// Conditional compilation is still very limited - there's no else
// and nesting is not possible. The following example shows all there is currently.

begin

{$ifdef ti99}
    writeln ('Hello, TI99');
    waitkey
{$endif}

{$ifdef fpc}
    writeln ('Hello, Linux')
{$endif}

end.
