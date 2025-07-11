unit unittest1;

interface

uses unittest2;

type 
    int = integer;

procedure print_unit1 (a: int);

implementation

type
    long = int64;

var
    h: int;
    l: long;

procedure print_unit1 (a: int);
    begin
	writeln ('Unit 1: ', a)
    end;
    
begin
    writeln ('Init Unit1')
end.
