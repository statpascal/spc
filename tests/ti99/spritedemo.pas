program spritedemo;

uses sprites;
    
const 
    nSprites = 16;
    
var 
    i: integer;
        
begin
    setVideoMode (StandardMode);
    setBackColor (black);
    copyPatternTables;
    setMagnification (true);
    setLargeSprites (true);
    
    for i := 0 to pred (nSprites) do
        begin
            createSprite (i, random (265), random (192), 128 + ord ('@') + 4 * random (6), TColor (2 + random (14)), boolean (random (2)));
            setMotion (i, 7 - random (15), 9 - random (18))
        end;
        
    setSpriteCount (nSprites);
    setMaxMotion (nSprites);
    
    waitKey
end.
