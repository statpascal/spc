program sockettest;

// open server to connect to with "netcat -l -p 12345"

uses cfuncs;

var
    s: int32;
    ip: uint32;
    saddr: sockaddr_in;
    msg: string;
    
begin
    ip := 127 shl 24 + 1;
    s := socket (AF_INET, SOCK_STREAM, 0);
    saddr.sin_family := AF_INET;
    saddr.sin_addr.s_addr := htonl (ip);
    saddr.sin_port := htons (12345);
    if connect (s, sockaddr (saddr), sizeof (saddr)) <> 0 then
        halt (1);
    msg := 'Hello from StatPascal';
    fdwrite (s, addr (msg [1]), length (msg));
    fdclose (s)
end.