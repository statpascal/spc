program resource;

uses vdp;

(* Embed two consecutive screen images (768 bytes each) into a resource file (resource.bin)
   that is linked to the program using an external declaration.
   
   When producing a bank switched cartridge, special care must be taken when passing the resource data 
   to other routines (e.g., writing to VDP RAM) as a bank switch can occur in the call.
   
   This program shows two possible ways to handle this:
   
   - copy the data to the stack before calling a runtime routine writing to VDP RAM (easy but time/memory 
     consuming)
   - copy a write function to the heap (a nested function is guaranteed to use a "near" call without 
     a bank switch) and access it using a routine pointer
*)     
   
type
    TWriteProc = procedure (src: pointer; size: integer) near;
    
var
    writeProc: TWriteProc;	// pointer to copy of vpdWrite on heap (in lower RAM)
    
procedure copyWriteProc;

    (* The following assembler routine is copied to a dynamically allocated area
       on the heap. Moving it is possible since only relative jumps are used. *)

    procedure vdpWrite (src: pointer; size: integer); assembler;
        limi 0
        
        li r13, >0040		// set VDP write addr to 0
        movb r13, @vdpwa
        swpb r13
        movb r13, @vdpwa        
        
        mov @src, r12
	mov @size, r13
	li r14, vdpwd
	
      vdpWrite1:
	movb *r12+, *r14
	dec 13
	jne vdpWrite1
	
	limi 2
    end;
    
    procedure vdpWriteEnd;
        begin
        end;
        
    var
        size, i: integer;
        p, q: ^char;
        
    begin
        q := addr (vdpWrite);
        size := integer (addr (vdpWriteEnd)) - integer (q);
        new (p, size);
        // we cannot use move from the runtime because that might switch banks
        for i := 0 to pred (size) do
            p [i] := q [i];
        writeProc := TWriteProc (p)
    end;

procedure pics; external 'resource.bin';

procedure showPics;
    const
	screenSize = 24 * 32;
    type 
        TScreenBuf = array [1..screenSize] of uint8;
        TScreenBuffers = array [0..1] of TScreenBuf;
    var
        picData: ^TScreenBuffers;
	buf: TScreenBuf;
    begin
        setVideoMode (StandardMode);
        setTextColor (black);
	picData := addr (pics);

        // create copy of image data on stack
        buf := picData^ [0];
	vmbw (buf, 0, screenSize);

	waitkey;
	
	// use near function copied to the heap
        writeProc (addr (picData^ [1]), screenSize);

	while keypressed do
    end;

begin
    copyWriteProc;
    showPics;
    waitkey
end.