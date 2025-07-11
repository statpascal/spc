program threadtest;

uses cthreads;


const 
    n = 16;
    m = 2;

var
    tid: array [1..n] of TThreadId;
    i: 1..n;
    data: int64;
    outMutex: TMutex;
    sem: TSemaphore;

procedure writeint64 (n: int64);
    begin
        lockmutex (outMutex);
        writeln (n);
        unlockmutex (outMutex)
    end;

function proc1 (p: pointer): ptrint;
    var
        i, sum: int64;
    begin
        sum := 0;
        for i := 1 to int64 (p^) do
            inc (sum, i);
        writeint64 (sum);
  	proc1 := 0
    end;

function proc2 (p: pointer): ptrint;
    var
        i, sum: int64;
        tid: TThreadId;
    begin
        beginthread (proc1, @data, tid);
        sum := 0;
        for i := 1 to data do
            inc (sum, i);
        writeint64 (sum);
        waitforthreadterminate (tid, 0);
	proc2 := 0
    end;

procedure test1;
    begin
        data := 10 * 1000;
 	initmutex (outMutex);
        for i := 1 to n div 2 do
            beginthread (proc1, @data, tid [i]);
        for i := n div 2 + 1 to n do
            beginthread (proc2, nil, tid [i]);
        for i := 1 to n do
            waitforthreadterminate (tid [i], 0);
	destroymutex (outMutex)
    end;


function proc3 (p: pointer): ptrint;
    const
        n = 20;
    var 
        i: 1..n;
    begin
        for i := 1 to n do
            begin
                acquireSemaphore (sem);
                write ('.');
                releaseSemaphore (sem)
            end;
	proc3 := 0;
    end;

procedure test2;
    begin
	initSemaphore (sem, 2);
	for i := 1 to n do
	    beginThread (proc3, nil, tid [i]);
        for i := 1 to n do
            waitforthreadterminate (tid [i], 0);
	destroySemaphore (sem);	
        writeln
    end;


begin
    test1;
    test2
end.

