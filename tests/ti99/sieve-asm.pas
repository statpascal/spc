program sieveasm (output);

const
    n = 1000;
    rep = 200;
    showPrimes = true;

var
    prim:  array [0..n + 1] of boolean;	// even size
    primend: integer;
    i, j, k, count: integer;

procedure doSieve; assembler;
        li   r0, prim
        li   r8, 501
        li   r13, >0101

    clear_1:
        mov  r13, *r0+
        dec  r8
        jne  clear_1

        li   r8, 2
        clr  r13

    primeloop_1:
        movb @prim(r8), r0
        jeq  done_1

        mov  r8, r12
        ai   r12, -1002

    loop_1:
        a     r8, r12
        jgt   done_1

        movb r13, @primend(r12)
        jmp  loop_1

    done_1:
        inc  r8
        ci   r8, 32
        jl   primeloop_1
end;

begin
    writeln ('Starting sieve');
    for k := 1 to rep do
        doSieve;
    count := 0;
    for i := 2 to n do
        if prim [i] then
            begin
                count := count + 1;
                if showPrimes then
                    begin
                        write (i:5);
                        if count mod 7 = 0 then
                            writeln
                    end
            end;
    writeln; writeln;
    writeln ('Found ', count, ' primes below ', n);
    waitkey
end.

