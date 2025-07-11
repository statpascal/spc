unit staticunit;

interface

const 
    b: real = 3.1415;
    
implementation

procedure test;
    const
        m: integer = 5;
    begin
        writeln ('m = ', m);
    end;

const
    s: string = 'Hello, world';
    
begin
    test;
    writeln ('s = ', s)
end.
