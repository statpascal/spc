unit unittest2;

interface

procedure print_unit2 (a: integer);

implementation

uses unittest3;

procedure print_unit2 (a: integer);
    begin
	writeln ('Unit 2: ', a)
    end;

begin
    unit3_hello;
    writeln ('Init Unit2')
end.
