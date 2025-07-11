program fortest;

const n = 10;

var
    sum: int64;
    i, j, k, l, m: int64;

begin
    sum := 0;
    for i := 1 to n do
        for j := 1 to n do
            begin
                k := i + j;
                l := i - j;
                m := i xor j;
                sum := sum - i * (sum - ((((j * (i xor (7 - k) + 3 * (3 - sum * 6)) xor (i + (k - 5))) xor (8 div j)) or (k and 7)) + (l and m)) xor i)
            end;
    writeln (sum)
end.
