unit vdp;

interface

type
    TVideoMode = (TextMode, StandardMode, BitmapMode);
    TColor = (transparent, black, mediumgreen, lightgreen, darkblue, lightblue, darkred,  cyan, 
              mediumred, lightred, darkyellow, lightyellow, darkgreen, magenta, gray, white);

var
    vdprd:  char absolute $8800;
    vdpsta: char absolute $8802;
    vdpwd:  char absolute $8c00;
    vdpwa:  char absolute $8c02;
    gromwa: char absolute $9c02;
    gromrd: char absolute $9800;
    
    imageTable, colorTable, patternTable,
    spriteAttributeTable, spritePatternTable: integer;
    
procedure vmbw (var src; dest, length: integer);	// video multiple byte write
procedure vmbr (var dest; src, length: integer);	// video multiple byte read
procedure vrbw (dest: integer; val: uint8; length: integer);		// video repeated byte write

procedure pokeV (addr: integer; val: uint8);
function peekV (addr: integer): uint8;
    
procedure setVideoMode (mode: TVideoMode);
function getVideoMode: TVideoMode;

procedure setBackColor (color: TColor);
procedure setTextColor (color: TColor);
procedure setColor (group: integer; fore, back: TColor);

procedure showHChar (x, y, val, count: integer);
procedure showVChar (x, y, val, count: integer);

type
    TCharPattern = string [16];

procedure setCharPattern (ch: uint8; pattern: TCharPattern);
function getCharPattern (ch: uint8): TCharPattern;

procedure gotoxy (x, y: integer);
function whereX: integer;
function whereY: integer;
function screenWidth: integer;

procedure clrscr;
procedure scroll;
procedure outputString (p: PChar; outlen: integer);
procedure outputLine;

// other low level routines

procedure setCRUBit (addr: integer; val: boolean);
procedure limi0;
procedure limi2;


implementation

const
    WriteAddr = $4000;

var
    vdpWriteAddress, imageTableEnd: integer;
    videoMode: TVideoMode;
    foreColor, backColor: TColor;
    
procedure setVdpAddress (n: integer);
    begin
        vdpwa := chr (n and 255);
        vdpwa := chr (n shr 8)
    end;
    
procedure vmbw (var src; dest, length: integer); assembler;
        mov @length, r14
        jeq vmbw_2
        
        limi 0
        mov @dest, r13
        ori r13, >4000
        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa
        
        mov @src, r12
        li r15, vdpwd

    vmbw_1:
        movb *r12+, *r15
        dec r14
        jne vmbw_1
        limi 2
        
    vmbw_2:
end;
    
procedure vmbr (var dest; src, length: integer); assembler;
        mov @length, r14
        jeq vmbr_2
        
        limi 0
        mov @src, r13
        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa
        
        li r15, vdprd
        mov @dest, r12

    vmbr_1:
        movb *r15, *r12+
        dec r14
        jne vmbr_1
        limi 2
        
    vmbr_2:
end;

procedure vrbw (dest: integer; val: uint8; length: integer); assembler;
        mov @length, r12
        jeq vrbw_2
        
        limi 0
        mov @dest, r13
        ori r13, >4000
        swpb r13
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa
        
        li r13, vdpwd
        movb @val, r14
    vrbw_1:
        movb r14, *r13
        dec r12
        jne vrbw_1
        limi 2
        
    vrbw_2:
end;

procedure loadCharSet (gromAddr, vdpAddr: integer);
    var 
        i, j: integer;
    begin
        limi0;
        gromwa := chr (gromAddr shr 8); 
        gromwa := chr (gromAddr and $ff);
        setVdpAddress (vdpAddr + 8 * 31 or WriteAddr);
        for i := 1 to 7 do
            vdpwd := #$3f;
        vdpwd := #0;
        for i := 32 to 127 do
            begin
                for j := 1 to 7 do
                    vdpwd := gromrd;
                vdpwd := #0
            end;
        limi2
    end;
    
procedure setVdpReg (nr, val: uint8);
    var
        dummy: char;
    begin
        limi0;
        dummy := vdpsta;
        vdpwa := chr (val);
        vdpwa := chr ($80 + nr);
        case nr of
            1:
                memb [$83d4] := val;		// Copy for KSCAN routine
            2:
                imageTable := $400 * (val and $0f);
            3:
                if videoMode = BitmapMode then
                    colorTable := $2000 * (val shr 7)
                else if videoMode = StandardMode then
                    colorTable := $40 * val
                else
                    colorTable := -1;
            4:
                if videoMode = BitmapMode then
                    patternTable := $2000 * (val and $07 shr 2)
                else
                    patternTable := $800 * (val and $07);
            5:
                if videoMode = TextMode then
                    spriteAttributeTable := -1
                else
                    spriteAttributeTable := $80 * (val and $7f);
            6:
                if videoMode = TextMode then
                    spritePatternTable := -1
                else
                    spritePatternTable := $800 * (val and $07);
            7:
                begin
                    foreColor := TColor (val shr 4);
                    backColor := TColor (val and $0f)
                end
        end;
        limi2
    end;
        
procedure pokeV (addr: integer; val: uint8);
    begin
        vmbw (val, addr, 1)
    end;
    
function peekV (addr: integer): uint8;
    begin
        vmbr (result, addr, 1)
    end;

procedure setVideoMode (mode: TVideoMode);    
    const
        vdpRegs: array [TVideoMode, 0..7] of uint8 = (
            ($00, $f0, $01, $00, $00, $00, $00, $17),
            ($00, $e0, $00, $0e, $01, $06, $00, $07),
            ($02, $e0, $06, $ff, $03, $78, $07, $00)
        );
    var
        i: integer;
        screen: array [0..255] of uint8;
    begin
        videoMode := mode;
        setVdpReg (1, $80);	// screen off
        for i := 2 to 7 do
            setVdpReg (i, vdpRegs [videoMode, i]);
        case videoMode of
            TextMode:
                begin
                    imageTableEnd := imageTable + 24 * 40;
                    loadCharSet ($06b4, patternTable);
                    clrscr;
                end;
            StandardMode:
                begin
                    imageTableEnd := imageTable + 24 * 32;
                    loadCharSet ($06b4, patternTable);
                    setTextColor (black);
                    clrscr;
                end;
            BitmapMode:
                begin
                    for i := 0 to 255 do
                        screen [i] := i;
                    vrbw (patternTable, 0, $1800);
                    for i := 0 to 2 do
                        vmbw (screen, imageTable + i * $100, $100);
                    vrbw (colorTable, 0, $1800);
                end
        end;
        if videoMode <> TextMode then
            pokeV (spriteAttributeTable, $d0);	// sprites off
        for i := 0 to 1 do
            setVdpReg (i, vdpRegs [videoMode, i])
    end;
    
function getVideoMode: TVideoMode;
    begin
        getVideoMode := videoMode
    end;
    
procedure setBackColor (color: TColor);
    begin
        setVdpReg (7, ord (foreColor) shl 4 or ord (color))
    end;
    
procedure setTextColor (color: TColor);
    var
        i: integer;
    begin
        setVdpReg (7, ord (color) shl 4 or ord (backColor));
        if videoMode = StandardMode then
            for i := 0 to 31 do
                setColor (i, color, transparent)
    end;
    
procedure setColor (group: integer; fore, back: TColor);
    begin
        pokeV (colorTable + group and $1f, ord (fore) shl 4 or ord (back))
    end;

procedure showHChar (x, y, val, count: integer);
    begin
        gotoxy (x, y);
        vrbw (vdpWriteAddress, val, min (count, imageTableEnd - vdpWriteAddress))
    end;
    
procedure showVChar (x, y, val, count: integer);
    var
        i: integer;
    begin
        for i := 0 to min (count, 24 - y) do
            begin
                gotoxy (x, y + i);
                write (chr (val))
            end
    end;

procedure setCharPattern (ch: uint8; pattern: TCharPattern);
    var
        chardef: array [0..7] of uint8;
        i: integer;
        
    function hexval (ch: char): integer;
        begin
            if ch in ['0'..'9'] then
                hexval := ord (ch) - ord ('0')
            else if ch in ['A'..'F'] then
                hexval := ord (ch) - ord ('A')
            else if ch in ['a'..'f'] then
                hexval := ord (ch) - ord ('a')
            else
                hexval := 0
        end;
        
    begin
        fillChar (pattern [succ (length (pattern))], pred (sizeof (pattern) - length (pattern)), 0);
        for i := 0 to 7 do
            chardef [i] := hexval (pattern [2 * i + 1]) * 16 + hexval (pattern [2 * i + 2]);
        vmbw (chardef, patternTable + 8 * ch, 8)
    end;
        
function getCharPattern (ch: uint8): TCharPattern;
    var
        chardef: array [0..7] of uint8;
        i: integer;
    begin
        vmbr (chardef, patternTable + 8 * ch, 8);
        result := '';
        for i := 0 to 7 do
            result := result + hexstr2 (chardef [i])
    end;

procedure _rt_scroll_up (start, stop, len, inc1, inc2: integer); assembler;
        mov @start, r0
        li r13, vdpwa
        li r14, vdprd
        li r15, vdpwd
        limi 0
        
    _rt_scroll_up_1:
        swpb r0
        movb r0, *r13
        swpb r0
        movb r0, *r13
        mov @len, r8
        li r12, >8320
        
    _rt_scroll_up_2:
        movb *r14, *r12+
        dec r8			// deley before accessing next byte
        movb *r14, *r12+
        dec r8
        movb *r14, *r12+
        dec r8
        movb *r14, *r12+
        dec r8
        jne _rt_scroll_up_2

        a @inc1, r0
        swpb r0
        movb r0, *r13
        swpb r0
        movb r0, *r13

        mov @len, r8
        li r12, >8320
        
    _rt_scroll_up_3:
        movb *r12+, *r15
        dec r8
        movb *r12+, *r15
        dec r8
        movb *r12+, *r15
        dec r8
        movb *r12+, *r15
        dec r8
        jne _rt_scroll_up_3

        a @inc2, r0
        c r0, @stop
        jl _rt_scroll_up_1

        mov @len, r8
        li r12, >2000
        
    _rt_scroll_up_4:
        movb r12, *r15
        dec r8
        jne _rt_scroll_up_4
        
        limi 2
end;

procedure gotoxy (x, y: integer);
    var
        offs: integer;
    begin
        if videoMode = TextMode then
            offs := (y shl 2 + y) shl 3
        else
            offs := y shl 5;
        vdpWriteAddress := min (imageTable + abs (offs + x), pred (imageTableEnd))
    end;
    
function whereX: integer;
    begin
        if videoMode = StandardMode then
            whereX := vdpWriteAddress and $1f
        else
            whereX := (vdpWriteAddress - imageTable) mod 40
    end;
    
function whereY: integer;
    begin
        if videoMode = StandardMode then
            whereY := (vdpWriteAddress - imageTable) shr 5
        else
            whereY := (vdpWriteAddress - imageTable) div 40
    end;
    
function screenWidth: integer;
    begin
        case videoMode of
            TextMode:
                screenWidth := 40;
            StandardMode:
                screenWidth := 32;
            BitmapMode:
                screenWidth := 256
        end
    end;
    
procedure clrscr;
    begin
        case videoMode of
            StandardMode:
                vrbw (imageTable, 32, 24 * 32);
            TextMode:
                vrbw (imageTable, 32, 24 * 40)
        end;
        gotoxy (0, 0)
    end;
    
procedure scroll;
    begin
        case videoMode of
            StandardMode:
                begin
                    _rt_scroll_up (imageTable + 32, imageTable + 24 * 32, 32, $3fe0, $c040);
                    dec (vdpWriteAddress, 32)
                end;
            TextMode:
                begin
                    _rt_scroll_up (imageTable + 40, imageTable + 24 * 40, 40, $3fd8, $c048);
                    dec (vdpWriteAddress, 40)
                end
        end
    end;
    
procedure outputString (p: PChar; outlen: integer);

    procedure __write_data (p: pointer; length: integer); assembler;
            mov @length, r12
            jeq __write_data_2
            
            limi 0
            
            mov @vdpWriteAddress, r13
            ori r13, >4000
            swpb r13
            movb r13, @vdpwa
            swpb r13
            movb r13, @vdpwa
            
            mov @p, r13
            li r14, vdpwd
            inc r13
        __write_data_1:
            movb *r13+, *r14
            dec r12
            jne __write_data_1
            limi 2
            
        __write_data_2:
    end;
    
    var
        len: integer;

    begin
        len := ord (p^);
        while vdpWriteAddress + outlen > imageTableEnd do
            scroll;
        if outlen > len then
            begin
                vrbw (vdpWriteAddress, 32, outlen - len);
                inc (vdpWriteAddress, outlen - len)
            end;
        __write_data (p, len);
        inc (vdpWriteAddress, len)
    end;
    
procedure outputLine;                
    begin
        case videoMode of
            TextMode:
                vdpWriteAddress := imageTable + 40 * ((vdpWriteAddress - (imageTable - 40)) div 40);
            StandardMode:
                vdpWriteAddress := (vdpWriteAddress + 32) and not 31
        end;
        while vdpWriteAddress >= imageTableEnd do
            scroll
    end;
    
procedure setCRUBit (addr: integer; val: boolean); assembler;
        mov  *r10, r12
        mov  @2(r10), r13
        ldcr r13, 1
end;

procedure limi0; assembler;
    limi 0
end;

procedure limi2; assembler;
    limi 2
end;
    
end.