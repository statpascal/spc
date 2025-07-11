program dist;

function l2_dist (a, b: realvector): real;
    begin
        l2_dist := sqrt (sum (sqr (a - b)))
    end;

begin
    writeln (l2_dist (combine (1, 1, 1), combine (2, 2, 2)), ' ', sqrt (3.0))
end.
