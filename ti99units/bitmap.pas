unit bitmap;

interface

type
    TColor = (transparent, black, mediumgreen, lightgreen, darkblue, lightblue, darkred,  cyan, 
              mediumred, lightred, darkyellow, lightyellow, darkgreen, magenta, gray, white);

procedure setBitmapMode;
procedure setStandardMode;

procedure setColor (color: TColor);
procedure setBkColor (color: TColor);

procedure plot (x, y: integer);
procedure line (x0, y0, x1, y1: integer);

implementation

const
    vdpRegsStandard: TVdpRegList = ($00, $c0, $00, $0e, $01, $06, $00, $07);
    vdpRegsBitmap: TVdpRegList = ($02, $c0, $06, $ff, $03, $78, $07, $00);
    
    imageTable = $1800;			// VR2 = $06
    patternTable = $0000;		// VR3 = $ff
    colorTable = $2000;			// VR4 = §03
    spriteAttributeTable = $3c00;	// VR5 = $78
    spritePatternTable = $3800;		// VR6 = $07
    
var
    activeColor: uint8;

procedure setBitmapMode;
    var 
        i: integer;
        screen: array [0..255] of uint8;
    begin
        for i := 0 to 255 do
            screen [i] := i;
        setVdpRegs (vdpRegsBitmap);
        vrbw (patternTable, 0, $1800);
        for i := 0 to 2 do
            vmbw (screen, imageTable + i * $100, $100);
        vrbw (ColorTable, activeColor, $1800);
        pokeV (spriteAttributeTable, $d0)	// sprites off
    end;
    
procedure setStandardMode;
    begin
        setVdpRegs (vdpRegsStandard);
        clrscr;
    end;

procedure setColor (color: TColor);
    begin
        activeColor := ord (color) shl 4;
    end;

procedure setBkColor (color: TColor);
    begin
        // write VDP reg
    end;
    
const
    invalid = -1;
    cachedChar: integer = invalid;
var
    cachePattern, cacheColor: array [0..7] of uint8;
    
procedure flushCache;
    begin
        vmbw (cachePattern, cachedChar, 8);
        vmbw (cacheColor, colorTable + cachedChar, 8)
    end;

procedure plot (x, y: integer);
    var
        offset: integer;
        y07: integer;
    begin
        y07 := y and $07;
        offset := y and $f8 shl 5 + x and $f8;
        if offset <> cachedChar then
            begin
                if cachedChar <> invalid then
                    flushCache;
                cachedChar := offset;
                vmbr (cachePattern, offset, 8);
                vmbr (cacheColor, colorTable + offset, 8);
            end;
        if activeColor = 0 then
            cachePattern [y07] := cachePattern [y07] and not ($80 shr (x and 7))
        else
            begin
                cachePattern [y07] := cachePattern [y07] or $80 shr (x and 7);
                cacheColor [y07] := activeColor
            end
(*            
        if activeColor = 0 then
            cachePattern [y and $07] := cachePattern [y and $07] and not ($80 shr (x and 7))
        else
            begin
                cachePattern [y and $07] := cachePattern [y and $07] or $80 shr (x and 7);
                cacheColor [y and $07] := activeColor
            end
*)            
    end;

// Bresenham's line algorithm
    
procedure line (x0, y0, x1, y1: integer);

    function sign (x: integer): integer;
        begin
            sign := ord (x > 0) - ord (x < 0)
        end;

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
        sy := sign (y1 - y0); 
        d := dy - dx;
        inc (dx, dx);

        for x := x0 to x1 do        
            begin
                if x_y then 
                    plot (y0, x) 
                else 
                    plot (x, y0);
                if (d > 0) or (d = 0) and (sy > 0) then 
                    begin
                        inc (y0, sy);
                        dec (d, dx)
                    end;
                inc (d, dy)
            end;
        flushCache
    end;

end.
