unit simpleunit;

interface

procedure Hello (var a: integer);


implementation

procedure Hello (var a: integer);
    begin
	writeln ('a = ', a)
    end;

end.