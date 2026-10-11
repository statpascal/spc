program resource;

uses vdp;

(* Embed two consecutive screen images (768 bytes each) in a resource file (resource.bin)
   that is linked to the program using an external declaration.

   When producing a bank switched cartridge, special care must be taken when passing the resource data
   to other routines (e.g., writing to VDP RAM) as a bank switch can occur in the call.

   Declaring procedure pics local to showPics guarantees that they are placed in the same bank,
   but a copy of the data is required before calling vmbw.

   Alternatively, one could provide a local "copy to VDP routine".
*)

procedure waitKeyPressed;
    begin
        while not keyPressed do;
        while keyPressed do
    end;

procedure showPics;
    const
        screenSize = 24 * 32;
    type
        TScreenBuf = array [0..screenSize - 1] of uint8;
        TScreenBuffers = array [0..1] of TScreenBuf;

    procedure pics; external 'resource.bin';

    var
        picData: TScreenBuffers absolute pics;
        buf: TScreenBuf;
    begin
        buf := picData [0];
        vmbw (buf, 0, screenSize);
        waitKeyPressed;
        buf := picData [1];
        vmbw (buf, 0, screenSize)
    end;

begin
    setVideoMode (StandardMode);
    setTextColor (black);
    showPics;
    waitkey
end.
