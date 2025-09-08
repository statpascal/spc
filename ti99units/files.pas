unit files;

interface

uses dsr;

procedure __assign (var f; filename: string);
procedure __rewrite_text (var f: text);
procedure __rewrite_bin (var f; blocksize: integer);
procedure __write_bin_ign (var f; var data; blocks: integer);
procedure __close (var f);

procedure __end_line (var f: text);

const
    TooManyOpenFiles = 1;
    FileOpenFailed = 2;

var
   InOutRes: integer;
   
function IOResult: integer;


implementation

const
    nFiles = 3;
    fileBufSize = 245;
    
type
    TFileDescriptor = record
        isOpen: boolean;
        pabAddr, bufAddr: integer
    end;
    
var
    vdpFree: integer absolute $8370;
    openFiles: array [1..nFiles] of TFileDescriptor;
    
function IOResult: integer;
    begin
        IOResult := InOutRes;
        InOutRes := 0
    end;
    
procedure reserveVdpBufs;
    var 
        i: integer;
    begin
        for i := 1 to nFiles do
            with openFiles [i] do
                begin
                    dec (vdpFree, fileBufSize);
                    isOpen := false;
                    bufAddr := succ (vdpFree);
                    dec (vdpFree, sizeof (TPab));
                    pabAddr := succ (vdpFree)
                end
    end;
        
procedure __assign (var f; filename: string);
    begin
        
        __file_data (f).pab.name := filename
    end;
    
function getFileDescriptor (var f: __file_data): boolean;
    var
        i: integer;
    begin
        f.fileidx := -1;
        f.bufpos := 0;
        fillChar (f.pab, 8, 0);
        
        if f.pab.name = '' then
            f.fileidx := 0
        else
            begin
                i := 1;
                repeat
                    with openFiles [i] do
                        if not isOpen then
                            begin
                                isOpen := true;
                                f.fileidx := i;
                                f.pab.vdpaddr := openFiles [f.fileidx].bufAddr
                            end;
                    inc (i)
                until (i > nFiles) or (f.fileidx <> -1)
            end;
        InOutRes := TooManyOpenFiles * ord (f.fileidx = -1);
        getFileDescriptor := InOutRes = 0
    end;
        
function execDsr (var f: __file_data): boolean;
    begin
        execDsr := dsrLink (f.pab, openFiles [f.fileidx].pabAddr)
    end;        
    

procedure __rewrite_bin (var f; blocksize: integer);
    begin
        if getFileDescriptor (__file_data (f)) and (__file_data (f).fileidx <> 0) then
            begin
                __file_data (f).pab.opcode := PabOpen;
                __file_data (f).pab.err_type := PabOutput or PabInternal or PabFixed;
                __file_data (f).pab.reclen := blocksize;
                InOutRes := FileOpenFailed * ord (not execDsr (__file_data (f)))
            end
    end;                
    
procedure __rewrite_text (var f: text);
    begin
        if getFileDescriptor (__file_data (f)) and (f.fileidx <> 0) then
            begin
                f.pab.opcode := PabOpen;
                f.pab.err_type := PabOutput or PabDisplay or PabVariable; 
                f.pab.reclen := 254;
                InOutRes := FileOpenFailed * ord (not (execDsr (__file_data (f))))
            end
    end;
    
procedure __write_bin_ign (var f; var data; blocks: integer);
    type
        arrtype = array [0..MaxInt] of uint8;
    var 
        I: integer;
    begin
        __file_data (f).pab.opcode := PabWrite;
        for i := 0 to pred (blocks) do
            begin
                vmbw (arrtype (data) [i * __file_data (f).pab.reclen], __file_data (f).pab.vdpaddr, __file_data (f).pab.reclen);
                execDsr (__file_data (f))
            end
    end;        
    
procedure __close (var f);
    begin
        InOutRes := 0;
        if __file_data (f).fileidx > 0 then
            begin
                __file_data (f).pab.opcode := PabClose;
                execDsr (__file_data (f));
                __file_data (f).fileidx := -1
            end
        else
            if __file_data (f).fileidx < 0 then
                InOutRes := 1
    end;
    
procedure __end_line (var f: text);
    begin
        if f.fileidx > 0 then    
            begin
                f.pab.opcode := PabWrite;
                f.pab.numchar := f.bufpos;
                execDsr (__file_data (f));
                f.bufpos := 0
            end
    end;

begin
    reserveVdpBufs
end.
