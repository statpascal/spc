program list;


type 
    content = string [20];
    
    nodeptr = ^node;
    node = record
        s: content;
        next: nodeptr
    end;
    
var
    list: nodeptr;
    
procedure insert (var p: nodeptr; s: content);
    var
        q: nodeptr;
    begin
        new (q);
        q^.s := s;
        q^.next := p;
        p := q
    end;
    
procedure traverse (p: nodeptr);
    begin
        while p <> nil do
            begin
                writeln (p^.s);
                p := p^.next
            end
    end;
    
begin
    list := nil;
    insert (list, 'First string');
    insert (list, 'Second string');
    insert (list, 'Final string');
    writeln ('Traversing list:');
    writeln;
    traverse (list);
    waitkey
end.
        