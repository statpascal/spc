n = 1000 * 1000

io.write (2, " ")
i = 3
while i <= n do
    j = 3
    prim = true
    while prim and j * j <= i do
        if i % j == 0 then
            prim = false
        else
            j = j + 2
        end 
    end
    if prim then
        io.write (i, ' ')
    end
    i = i + 2
end
