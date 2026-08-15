unit sprites;

// Must be compiled with interrupts enabled

interface

uses vdp;

procedure createSprite (spriteNr, x, y, pattern: uint8; color: TColor; clock: boolean);

procedure clearMotionTable;
procedure setMotion (spriteNr, dx, dy: uint8);

procedure setMagnification (f: boolean);
procedure setLargeSprites (f: boolean);

procedure setMaxMotion (nr: uint8);
procedure setSpriteCount (n:  uint8);

procedure copyPatternTables;


implementation

uses vdp;

const
    SpriteMotionTable = $0780;

procedure clearMotionTable;
    begin
        vrbw (SpriteMotionTable, 0, $80)
    end;
    
procedure setMaxMotion (nr: uint8);
    begin
        memB [$837a] := nr
    end;
    
procedure setSpriteCount (n:  uint8);
    begin
        if n < 32 then
            pokeV (spriteAttributeTable + 4 * n, $d0)
    end;

procedure setMotion (spriteNr, dx, dy: uint8);
    type
        TSpriteMotion = record
            deltaY, deltaX: int8;
            internal: int16
        end;
    var
        motion: TSpriteMotion;
    begin
        with motion do 
            begin
                deltaY := dy;
                deltaX := dx;
                internal := 0
            end;
        vmbw (motion, SpriteMotionTable + 4 * spriteNr, 4)
    end;
        
procedure createSprite (spriteNr, x, y, pattern: uint8; color: TColor; clock: boolean);
    type
        TSpriteAttributes = record
            posY, posX, pat, clockColor: uint8
        end;
    var
        attributes: TSpriteAttributes;
    begin
        with attributes do
            begin
                posY := y;
                dec (posY);
                posX := x;
                pat := pattern;
                clockColor := ord (color);
                if clock then
                    begin
                        clockColor := clockColor or $80;
                        inc (posY, 32)
                    end
            end;
        vmbw (attributes, spriteAttributeTable + 4 * spriteNr, 4)
    end;

procedure copyPatternTables;
    const
        size = 1024;
    var
        buf: array [0..size - 1] of uint8;
    begin
        vmbr (buf, patternTable, size);
        vmbw (buf, spritePatternTable + size, size)
    end;
    
procedure setFlag (val: uint8; f: boolean);
    begin
        if f then
            setVdpReg (1, getVdpReg (1) or val)
        else
            setVdpReg (1, getVdpReg (1) and not val)
    end;
    
procedure setMagnification (f: boolean);
    begin
        setFlag ($01, f)
    end;
     
procedure setLargeSprites (f: boolean);
    begin
        setFlag ($02, f)
    end;
    
begin
end.
