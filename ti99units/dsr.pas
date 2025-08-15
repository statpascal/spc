unit dsr;

interface

type
    TStandardHeaderNodePtr = ^TStandardHeaderNode;
    TStandardHeaderNode = record
        next: TStandardHeaderNodePtr;
        address: integer;
        name: string
    end;
    
    TStandardHeader = record
        magic, version, nrprogs, filler: uint8;
        pwrupList: pointer;
        prgList, dsrList, subList: TStandardHeaderNodePtr;
        isrList: pointer
    end;

    TPab = record
        opcode, err_type: uint8;
        vdpaddr: integer;
        reclen, numchar: uint8;
        recnr: integer;
        status: uint8;
        name: string [25]	// TODO:  maximum file name length?
    end;

function dsrLink (var pab: TPab; pabVdpAddr: integer): boolean;

const
    PabStatusFileNotFound = $80;
    PabStatusWriteProtected = $40;
    PabStatusFileInternal = $10;
    PabStatusFileProgram = $08;
    PabStatusFileVariable = $04;
    PabStatusMemoryFull = $02;
    PabStatusEOFReached = $01;

    PabOpen = 0;
    PabClose = 1;
    PabRead = 2;
    PabWrite = 3;
    PabRewind = 4;
    PabLoad = 5;
    PabSave = 6;
    PabDelete = 7;
    PabScratch = 8;
    PabStatus = 9;
    PabOpenInterrupt = $80;

    PabFixed = 0;
    PabVariable = $10;
    
    PabDisplay = 0;
    PabInternal = $08;
    
    PabUpdate = 0;
    PabOutput = 2;
    PabInput = 4;
    PabAppend = 6;
    
    PabSequential = 0;
    PabRelative = 1;
    
    PabNoError = 0;
    PabWriteProtection = 1;
    PabBadAttribute = 2;
    PabIllegalOpcode = 3;
    PabMemoryFull = 4;
    PabPastEOF = 5;
    PabDeviceError = 6;
    PabFileError = 7;


implementation

var 
    header: TStandardHeader absolute $4000;
        
procedure calldsr (var res: boolean); assembler;
        lwpi >83e0	// GPLWS

	bl *r9
	jmp calldsr_1	// continue search for DSR

	lwpi >8300
	li r0, 256	// call completed
	jmp calldsr_2
	
    calldsr_1:
	lwpi >8300	// TODO: keep variable/configurable
	clr r0
	
    calldsr_2:
	mov @res, r12
	movb r0, *r12
end;


function dsrLink (var pab: TPab; pabVdpAddr: integer): boolean;
    var 
        cruAddr, len: integer;
        dsrList: TStandardHeaderNodePtr;
        completed: boolean;
        device: string [7];
    begin
        len := pred (pos ('.', pab.name));
        if len = -1 then
            len := length (pab.name);
        device := copy (pab.name, 1, len);

        vmbw (pab, pabVdpAddr, sizeof (TPab));
        memW [$8356 div 2] := pabVdpAddr + 10 + len;
        memW [$8354 div 2] := len;
        
        // make sure all cards are turned off
        for cruAddr := $10 to $1f do
            setCRUBit (cruAddr shl 8, false);
        
        cruAddr := $1000;
        completed := false;
        repeat
            setCRUBit (cruAddr, true);
            if header.magic = $aa then
                begin
                    dsrList := header.dsrList;
                    while (dsrList <> nil) and not completed do
                        begin
                            if dsrList^.name = device then
                                begin
                                    memW [$83f2 div 2] := dsrList^.address;	// R9 (GPLWS)
                                    memW [$83f8 div 2] := cruAddr;		// R12 (GPLWS)

(* The DSRLNK routine in the Editor/Assembler module would also set the following memory
   addresses (not saving R15 of GPLWS). It is unclear if that is really needed - at least
   the orignal disk DSR does not seem to require it:
                                    
                                    memW [$837c shr 1] := 0;
                                    memW [$83d0 shr 1] := cruAddr;
                                    memW [$83d2 shr 1] := integer (dsrList);
                                    memW [$8e02 shr 1] := 1;	// R1 (GPLWS)
                                    saveR15 := memW [$83fe shr 1];
                                    memW [$83fe shr 1] := $8c02;
                                    -- restore R15 (GPLWS) after call
                                    memW [$83fe shr 1] := saveR15
*)                                    
                                    calldsr (completed)
                                end;
                            dsrList := dsrList^.next
                        end
                end;
            setCRUBit (cruAddr, false);
            inc (cruAddr, $100)
        until (cruAddr = $2000) or completed;
        
        vmbr (pab, pabVdpAddr, sizeof (TPab));
        dsrLink := completed
    end;

end.
