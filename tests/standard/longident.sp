program longident;

type
    rec = record
        this_is_a_very_long_identifier_name: string;
    end;

var
    a: rec;

begin
    a.this_is_a_very_long_identifier_name := 'String';
    writeln (a.this_is_a_very_long_identifier_name)
end.
