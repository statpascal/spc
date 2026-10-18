unit bitmap;

interface

uses vdp;

procedure setForeColor (color: TColor);

procedure plot (x, y: integer);
procedure line (x0, y0, x1, y1: integer);

implementation

var
    activeColor: uint8;

procedure setForeColor (color: TColor);
    begin
        activeColor := ord (color) shl 4;
    end;

const
    invalid = -1;
    cachedChar: integer = invalid;

var
    cachePattern, cacheColor: array [0..7] of uint8;

{$bank:on}

procedure writeCache near; assembler;
        ori  r13, >4000
        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa

        li   r13, vdpwd
        movb *r14+, *r13
        movb *r14+, *r13
        movb *r14+, *r13
        movb *r14+, *r13
        movb *r14+, *r13
        movb *r14+, *r13
        movb *r14+, *r13
        movb *r14, *r13
end;

procedure readCache near; assembler;
        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa

        li   r13, vdprd
        movb *r13, *r14+
        movb *r13, *r14+
        movb *r13, *r14+
        movb *r13, *r14+
        movb *r13, *r14+
        movb *r13, *r14+
        movb *r13, *r14+
        movb *r13, *r14
end;

procedure transferCache (f: pointer) near; assembler;
        mov  r11, r12
        mov  @f, r15

        limi 0

        mov  @patternTable, r13
        a    @cachedChar, r13
        li   r14, cachePattern
        bl   *r15

        mov  @colorTable, r13
        a    @cachedChar, r13
        li   r14, cacheColor
        bl   *r15

        movb @interruptsEnabled, r13
        jeq  transfercache_1
        limi 2

    transfercache_1:
        mov  r12, r11
end;

procedure plot_near (x, y: integer) near;
    var
        offset: integer;
        y07: integer;
    begin
        y07 := y and $07;
        offset := y and $f8 shl 5 + x and $f8;
        if offset <> cachedChar then
            begin
                if cachedChar <> invalid then
                    transferCache (addr (writeCache));
                cachedChar := offset;
//                vmbr (cachePattern, patternTable + offset, 8);
//                vmbr (cacheColor, colorTable + offset, 8);
                transferCache (addr (readCache))
            end;
        if activeColor = 0 then
            cachePattern [y07] := cachePattern [y07] and not ($80 shr (x and 7))
        else
            begin
                cachePattern [y07] := cachePattern [y07] or $80 shr (x and 7);
                cacheColor [y07] := activeColor
            end
    end;

// Bresenham's line algorithm

procedure line (x0, y0, x1, y1: integer);

    procedure swap (var a, b: integer);
        var
            h: integer;
        begin
            h := a; a := b; b := h
        end;

    var
        dx, dy, sy, d, x: integer;
        x_y : boolean;

    begin
        x_y := abs (y1 - y0) > abs (x1 - x0);
        if x_y then
            begin
                swap (x0, y0);
                swap (x1, y1)
            end;
        if x1 < x0 then
            begin
                swap (x0, x1);
                swap (y0, y1)
            end;
        dx := x1 - x0;
        dy := abs (y1 - y0) shl 1;
        sy := ord (y1 > y0) - ord (y1 < y0);	// sign (y1 - y0);
        d := dy - dx;
        inc (dx, dx);

        for x := x0 to x1 do
            begin
                if x_y then
                    plot_near (y0, x)
                else
                    plot_near (x, y0);
                if (d > 0) or (d = 0) and (sy > 0) then
                    begin
                        inc (y0, sy);
                        dec (d, dx)
                    end;
                inc (d, dy)
            end;
        transferCache (addr (writeCache))
    end;

procedure plot (x, y: integer);
    begin
        plot_near (x, y)
    end;

{$bank:off}

end.
