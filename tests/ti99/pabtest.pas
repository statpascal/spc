program pabtest;

uses dsr;

procedure test;
    const
        pabVdpAddr = $2000;
        bufVdpAddr = $2100;
    var
        pab: TPab;
        s: string;
    begin
        with pab do begin
            opcode := PabOpen;
            err_type := PabOutput or PabDisplay or PabVariable; 
            vdpaddr := bufVdpAddr;
            reclen := 80;
            numchar := 0;
            status := 0;
            recnr := 0;
            name := 'DSK1.TEST01'
        end;
            
        if not dsrLink (pab, pabVdpAddr) then
            begin
                writeln ('Cannot find DSR for DSK1 - quitting');
                exit
            end
        else
            writeln ('Success: ', pab.name , ' opened');
        writeln ('Status: ', pab.status);
            
        // write some text
        s := 'Test output to file';
        vmbw (s [1], bufVdpAddr, length (s));
        pab.opcode := PabWrite;
        pab.numchar := length (s);
        dsrLink (pab, pabVdpAddr);
        writeln ('Status: ', pab.status);

        // write it again
        inc (pab.recnr);
        dsrLink (pab, pabVdpAddr);
        writeln ('Status: ', pab.status);
        
        // close file
        pab.opcode := PabClose;
        dsrLink (pab, pabVdpAddr);
        writeln ('Status: ', pab.status)
    end;
    
begin
    test;
    waitkey
end.
