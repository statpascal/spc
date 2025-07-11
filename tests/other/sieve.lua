n = 10 * 1000 * 1000

prim = {}
prim [1] = false
prim [2] = true

for i = 3, n do
    prim [i] = (i % 2 == 1)
end

i = 3
while i * i <= n do
    if prim [i] then
        j = 2 * i
        while j <= n do
            prim [j] = false
            j = j + i
        end
    end
    i = i + 2
end

for i = 1, n do
    if prim [i] then
        io.write (i, " ")
    end
end

