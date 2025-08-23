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

implementation

const
    vdpRegsStandard: TVdpRegList = ($00, $c0, $00, $0e, $01, $06, $00, $07);
    vdpRegsBitmap: TVdpRegList = ($02, $c0, $06, $ff, $03, $00, $00, $00);
    
    patternTable = $0000;
    colorTable = $2000;
    
var
    activeColor: uint8;

procedure setBitmapMode;
    var 
        i, j: integer;
    begin
        setVdpRegs (vdpRegsBitmap);
        setVdpAddress (WriteAddr);
        vrbw (0, $1800);
        for i := 1 to 3 do
            for j := 0 to 255 do
                vdpwd := chr (j);
        setVdpAddress (ColorTable or WriteAddr);
        vrbw (activeColor, $1800)
    end;
    
procedure setStandardMode;
    begin
        setVdpRegs (vdpRegsStandard);
        clrscr;
    end;

procedure setColor (color: TColor);
    begin
        activeColor := ord (color) shl 4 or activeColor and $0f
    end;

procedure setBkColor (color: TColor);
    begin
        activeColor := activeColor and $f0 or ord (color)
    end;

procedure plot (x, y: integer);
    const
        invalid = -1;
        cachedChar: integer = invalid;
        cache: array [0..7] of uint8 = (0, 0, 0, 0, 0, 0, 0, 0);
    var
        offset: integer;
    begin
        offset := y and $f8 shl 5 + x and $f8;
        if offset <> cachedChar then
            begin
                if cachedChar <> invalid then
                    vmbw (cache, cachedChar, 8);
                cachedChar := offset;
                vmbr (cache, offset, 8)
            end;
        cache [y and $07] := cache [y and $07] or $80 shr (x and 7)
        // we also need to set the color at this point
    end;
    
end.
