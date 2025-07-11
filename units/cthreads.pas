unit cthreads;

interface

type 
    TThreadId = int64;
    ptrint = int64;
    TThreadProc = function (data: pointer): ptrint;
    
    (* distinct pointer types as handles to the underlying data *)
    TMutex = ^record end;
    TSemaphore = ^record end;
    
procedure initMutex (var m: TMutex); external name 'rt_mutex_init';
procedure lockMutex (m: TMutex); external name 'rt_mutex_lock';
procedure unlockMutex (m: TMutex); external name 'rt_mutex_unlock';
procedure destroyMutex (m: TMutex); external name 'rt_mutex_destroy';

procedure initSemaphore (var s: TSemaphore; init: int32); external name 'rt_sem_init';
procedure acquireSemaphore (s: TSemaphore); external name 'rt_sem_acquire';
function acquireSemaphore (s: TSemaphore; millisecs: uint32): boolean; external name 'rt_sem_acquire_timeout';
procedure releaseSemaphore (s: TSemaphore); external name 'rt_sem_release';
procedure destroySemaphore (s: TSemaphore); external name 'rt_sem_destroy';

(* Compatibility with FPC *)

type
    TRtlCriticalSection = TMutex;
    PRtlEvent = TSemaphore;

procedure beginThread (p: TThreadProc; data: pointer; var t: TThreadId); external name 'rt_thread_create';
procedure waitForThreadTerminate (t: TThreadId; TimeOutMs: int64); external name 'rt_thread_join';

procedure initCriticalSection (var cs: TRtlCriticalSection); external name 'rt_mutex_init';
procedure enterCriticalSection (cs: TRtlCriticalSection); external name 'rt_mutex_lock';
procedure leaveCriticalSection (cs: TRtlCriticalSection); external name 'rt_mutex_unlock';
procedure doneCriticalSection (cs: TRtlCriticalSection); external name 'rt_mutex_destroy';

function RtlEventCreate: PRtlEvent; // external name 'rt_event_init';
procedure RtlEventDestroy (p: PRtlEvent); // external name 'rt_event_destroy';
procedure RtlEventWaitFor (p: PRtlEvent); // external name 'rt_event_wait';
procedure RtlEventSetEvent (p: PRtlEvent); // external name 'rt_event_signal';


implementation

function RtlEventCreate: PRtlEvent;
    var
        res: PRtlEvent;
    begin
        initSemaphore (res, 0);
        RtlEventCreate := res
    end;
    
procedure RtlEventDestroy (p: PRtlEvent);
    begin
        destroySemaphore (p)
    end;
    
procedure RtlEventWaitFor (p: PRtlEvent); 
    begin
        acquireSemaphore (p)
    end;
    
procedure RtlEventSetEvent (p: PRtlEvent);
    begin
        releaseSemaphore (p)
    end;

end.
