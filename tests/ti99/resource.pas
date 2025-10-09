program resource;

// embed resource into program

procedure p;

    (*
	The external data block (procedure q) and the nested
        subroutines accessing it are put into the same bank.

        Calling vmbw from system.pas requires a copy to RAM since a bank
        switch can occur.

        Using the local vdpWrite, direct access to the data is possible.
    *)

    procedure q; external 'resource.bin';

    procedure vdpWrite (src, size: integer); assembler;
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

    const
	screenSize = 24 * 32;
    var
	buf: array [1..screenSize] of uint8;
    begin
	move (addr (q)^, buf, screenSize);
	vmbw (buf, 0, screenSize);
	
	waitkey;
        vdpwrite (integer (addr (q) + screenSize), screenSize);

	repeat
	until not keypressed
    end;

begin
    p;
    waitkey
end.