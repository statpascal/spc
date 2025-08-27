program strings;

const
    s = 'String constant';
    t = 'String also';

var
    a, b: string [20];
    ch: char;
    i: integer;
    
begin
    a := 'Hello, ';
    b := 'world!';
    writeln (a + b);
    
    a := '';
    writeln ('Length of empty string: ', length (a));
    
    // Truncated after 20 characters:
    for ch := 'A' to 'Z' do
        a := a + ch;
    writeln (length (a), ': ', a);    
    writeln ('Also length: ', ord (a [0]));
    
    a [0] := #5;
    writeln ('Truncated: ', a);
    
    writeln (length (s), ': ', s);
    writeln (copy (s, 1, 6));
    writeln (copy (s, 8, 100));
    
    a [0] := chr (10);
    for i := 1 to 10 do
        a [i] := chr (ord ('a') + pred (i));
    writeln (a);

    writeln;
    writeln ('Comparison:');
    writeln ('s=', s);
    writeln ('t=', t);    
    writeln ('s=s: ', s = s);
    writeln ('s=t: ', s = t);
    writeln ('s<t: ', s < t);
    writeln ('t<s: ', t < s);
    
    waitkey
end.