program tree;


type 
    content = integer;
    
    nodeptr = ^node;
    node = record
        s: content;
        left, right: nodeptr
    end;
    
procedure insert (var p: nodeptr; s: content);
    begin
        if p = nil then
            begin
                new (p);
                p^.left := nil;
                p^.right := nil;
                p^.s := s
            end
        else if s < p^.s then
            insert (p^.left, s)
        else
            insert (p^.right, s)
    end;
    
procedure traverse (p: nodeptr);
    begin
        if p <> nil then
            begin
                traverse (p^.left);
                write (p^.s: 4);
                traverse (p^.right)
            end
    end;
    
var
    tree: nodeptr;
    i, val: integer;
    
begin
    tree := nil;
    writeln ('Inserting values:');
    for i := 1 to 40 do
        begin
            val := i * 37 mod 100;
            write (val:4);
            insert (tree, val)
        end;
    writeln; writeln;
    writeln ('Sorted values:');
    traverse (tree);
    waitkey
end.
        