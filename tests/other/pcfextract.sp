program pcfextract;

(* Extract bitmaps of fonts *)

const
    nmax = 1000 * 1000;

    magic: array [0..3] of char = (#01, 'f', 'c', 'p');

    Pcf_Properties = 1;
    Pcf_Accelerators = 2;
    Pcf_Metrics = 4;
    Pcf_Bitmaps = 8;
    Pcf_Bdf_Encoding = 32;

    Pcf_Compressed = $100;
    Pcf_BigEndian = $04;
    Pcf_LSBitFirst = $08;

    InvalidGlyphIndex = $ffff;

type
    PData = ^uint8;
    PUint32 = ^uint32;

    TFileHeader = record
	header: array [0..3] of char;
	table_count: uint32;
	tables: array [0..nmax] of record
	    entrytype, format, size, offset: uint32
	end
    end;
    TFontMetrics = record
	count: uint32;
	maxWidth, maxAscent, maxDescent: int16;
	metrics: array [0..nmax] of record
 	    leftSidedBearing, rightSideBearing, width, ascent, descent: int16
	end
    end;
    TEncodings = record
	minCharOrByte2, maxCharOrByte2, minByte1, maxByte1, defaultChar: uint16;
	glyphCount: uint16;
	glyphIndices: array [0..nmax] of uint16
    end;
    TBitmaps = record
	padding, storageSize: uint8;
	bigEndian, lsBitFirst: boolean;
	count: uint32;
	bitmap: PData;
	offsets: array [0..nmax] of uint32;
    end;

    PTFontMetrics = ^TFontMetrics;
    PTEncodings = ^TEncodings;
    PTBitmaps = ^TBitmaps;

    TExtractedFont = record
	fontMetrics: PTFontMetrics;
	encodings: PTEncodings;
        bitmaps: PTBitmaps;
	maxWidth, maxAscent, maxDescent: uint16;
    end;


function readNextValue (var p: PData; size: uint8; bigEndian: boolean): int64;
    var 
        res: int64;
	i: uint8;
    begin
	res := 0;
	for i := 0 to pred (size) do
	    inc (res, p [i] shl (8 * (i * ord (not bigEndian) + (pred (size) - i) * ord (bigEndian))));
	inc (p, size);
	readNextValue := res
    end;
   
procedure handleMetrics (metricsData: PData; var fontMetrics: PTFontMetrics);
    var
	n, i, format: uint32;
	compressed, bigEndian: boolean;

    function getValue: int64;
	begin
            if compressed then
		getValue := readNextValue (metricsData, 1,false) - $80
	    else
		getValue := readNextValue (metricsData, 2, bigEndian)
    	end;

    function max (a, b: int16): int16;
	begin
	    if a > b then
		max := a
	    else
		max := b
	end;
   
    begin
	format := readNextValue (metricsData, 4, false);
	compressed := format and Pcf_Compressed <> 0;
	bigEndian := format and Pcf_BigEndian <> 0;
        n := readNextValue (metricsData, 4 - 2 * ord (compressed), bigEndian);
	getmem (fontMetrics, 8 + 5 * n * sizeof (int16));
	with fontMetrics^ do
	    begin
		count := n;
		maxWidth := 0;
		maxAscent := 0;
		maxDescent := 0;
		for i := 0 to pred (count) do 
 		    with metrics [i] do
	  	        begin
  		            leftSidedBearing := getValue;
	        	    rightSideBearing := getValue;
		            width := getValue;
		            ascent := getValue;
		            descent := getValue;
			    maxWidth := max (width, maxWidth);
			    maxAscent := max (ascent, maxAscent);
			    maxDescent := max (descent, maxDescent)
	        	end
	    end
    end;

procedure handleEncodings (encodingData: PData; var encodings: PTEncodings);
    var
	min2, max2, min1, max1: uint16;
	count, i: uint32;
        format: uint32;
	bigEndian: boolean;
    begin
	format := readNextValue (encodingData, 4, false);
	writeln ('// Format: ', format);
	bigEndian := format and Pcf_BigEndian <> 0;
	writeln ('// Big endian: ', bigEndian);
	min2 := readNextValue (encodingData, 2, bigEndian);
	max2 := readNextValue (encodingData, 2, bigEndian);
	min1 := readNextValue (encodingData, 2, bigEndian);
	max1 := readNextValue (encodingData, 2, bigEndian);
	count := (max1 - min1 + 1) * (max2 - min2 + 1);
	writeln ('// Glyphs: ', count);
        getmem (encodings, (count + 6) * sizeof (uint32));
	with encodings^ do 
	    begin
		minCharOrByte2 := min2;
		maxCharOrByte2 := max2;
		minByte1 := min1;
		maxByte1 := max1;
		defaultChar := readNextValue (encodingData, 2, bigEndian);
		glyphCount := count;
		for i := 1 to count do 
		    glyphIndices [pred (i)] := readNextValue (encodingData, 2, bigEndian)
	    end;
	writeln ('// Min char: ', min2, ', max char: ', max2)
    end;

procedure handleBitmaps (bitmapData: PData; var bitmaps: PTBitmaps);
    var
	format, glyphCount: uint32;
	bigEndian: boolean;
	i: int64;
    begin
	format := readNextValue (bitmapData, 4, false);
	bigEndian := format and Pcf_BigEndian <> 0;
	glyphCount := readNextValue (bitmapData, 4, bigEndian);
        getmem (bitmaps, 16 + glyphCount * sizeof (uint32));
	bitmaps^.bigEndian := bigEndian;
	with bitmaps^ do 
	    begin
		lsBitFirst := format and Pcf_LSBitFirst <> 0;
		padding := 1 shl (format and 3);
		storageSize := 1 shl ((format shr 4) and 3);
		count := glyphCount;
		for i := 0 to pred (glyphCount) do
		    offsets [i] := readNextValue (bitmapData, 4, bigEndian);
		bitmap := bitmapData + 4 * sizeof (uint32);
	    end
    end;

procedure parseFile (pcfData: PData; var extractedFont: TExtractedFont);
    var
	fileHeader: ^TFileHeader absolute pcfData;
	i: uint32;

    begin
        for i := 0 to pred (fileHeader^.table_count) do
	    with fileHeader^.tables [i], extractedFont do
    	        begin
		    writeln ('// i: ', i, ', offset: ', offset, ', type: ', entrytype, ', size: ', size);
 		    case entrytype of
			Pcf_Metrics:
			    handleMetrics (pcfData + offset, fontMetrics);
			Pcf_Bitmaps:
			    handleBitmaps (pcfData + offset, bitmaps);
			Pcf_Bdf_Encoding:
			    handleEncodings (pcfData + offset, encodings)
		    end
		end
    end;

procedure readFile (fn: string; var data: PData);
    var
	f: file;
	fsize, read: int64;
    begin
	data := nil;
	if fileexists (fn) then 
	    begin
		assign (f, fn);
		reset (f, 1);
		fsize := filesize (f);
		if fsize > 0 then
		    begin
			getmem (data, fsize);
			blockread (f, data^, fsize, read);
			if read <> fsize then 
			    begin
   			        writeln ('Could only read ', read, ' bytes of total ', fsize, ' from ', fn);
			        freemem (data, fsize);
				data := nil
			    end
		    end
		else
		    writeln ('File ', fn, ' is empty or cannot be accessed.')
	    end
	else
	    writeln ('File ', fn, ' not found or cannot be accessed.')
    end;

procedure extractCharacter (ch: char; extractedFont: TExtractedFont; data: PUint32);
    var
	glyphIndex, i, j, k, bytesPerLine: uint16;
	val, mask: uint32;
	glyphData: PData;
    begin
	with extractedFont, fontMetrics^ do
	    begin
   	        fillChar (data^, 0, sizeof (uint32) * (maxAscent + maxDescent));
		glyphIndex := InvalidGlyphIndex;
		if encodings^.minByte1 = 0 then
		    glyphIndex := encodings^.glyphIndices [ord (ch) - encodings^.minCharOrByte2];
		if glyphIndex <> InvalidGlyphIndex then
		    with metrics [glyphIndex], bitmaps^ do
 		        begin
			    for i := ascent + 1 to maxAscent do 
				begin
				    data^ := 0;
				    inc (data)
				end;
			    glyphData := bitmap + offsets [glyphIndex];
			    bytesPerLine := (width div 8 + padding) and not pred (padding);
                            for i := 1 to ascent + descent do
				begin
				    data^ := 0;
				    mask := 1 shl 31;
				    for j := 1 to bytesPerLine div storageSize do
					begin
					    val := readNextValue (glyphData, storageSize, bigEndian);
					    for k := 1 to 8 * storageSize do
						begin
	 					    if lsBitFirst then
						        begin
							    if val and (1 shl (8 * storageSize - 1)) <> 0 then
							        data^ := data^ or mask;
							    val := val shl 1
						        end
						    else
						        begin
							    if val and 1 <> 0 then 
							        data^ := data^ or mask;
							    val := val shr 1
						        end;
					            mask := mask shr 1
						end
					end;
				    inc (data)
				end
			end
	    end			    
    end;
				

procedure dumpCharset (extractedFont: TExtractedFont);
    var
	data: PUint32;
	ch: char;
	i, j: int8;

    const 
        hex: array [0..15] of string = ('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F');
    

    function hexstr (u: uint8): string;    
        begin
            hexstr := hex [(u shr 4) and $0f] + hex [u and $0f]
        end;

    procedure dump (val: uint32; lastLine: boolean);
	var 
	    i: int8;
	begin
            write ('         {');
	    j := 31;
	    for i := pred (extractedFont.fontMetrics^.maxWidth) downto 0 do
                begin
                    write (ord (val and (1 shl j) <> 0));
		    if i > 0 then
		        write (',');
		    dec (j)
		end;
	    if lastLine then
	        writeln ('}')
	    else 
                writeln ('},')
	end;

    begin
        with extractedFont.fontMetrics^ do
	    begin
                writeln ('// Font size: max width: ', maxWidth, ', max ascent: ', maxAscent, ', max descent: ', maxDescent);
                writeln;
	        writeln ('const int FontWidth = ', maxWidth, ';');
                writeln;
	        writeln ('using TFontMatrix = std::vector<std::array<int, FontWidth>>;');
	        writeln ('using TFontMap = std::map<char, TFontMatrix>;');
	        writeln;
	        writeln ('TFontMap fontMap = {');
		getmem (data, sizeof (uint32) * (maxAscent + maxDescent));
		for ch := chr (32) to chr (127) do
		    begin
			extractCharacter (ch, extractedFont, data);
 	
			writeln ('    {', ord (ch), ', {{  // ' + ch );
			for i := 0 to pred (maxAscent + maxDescent) do
  			        dump (data [i], i = pred (maxAscent + maxDescent));
			if ch <> chr (127) then
			    writeln ('    }}},')
			else
			    writeln ('    }}}')
(*
			write ('        byte ');
			for i := 0 to pred (maxAscent + maxDescent) do
			    begin
 			        write ('>', hexstr (data [i] shr 24));
				if i <> pred (maxAscent + maxDescent) then write (', ')
			    end;
			writeln ('   ; ', ord (ch) , ': ', ch)
*)
		    end
	    end;
	    writeln ('};');
    end;

procedure decodeFont (fn: string);
    var
	data: PData;
	extractedFont: TExtractedFont;
    begin
	readFile (fn, data);
	if data <> nil then
	    begin
		parseFile (data, extractedFont);
		dumpCharset (extractedFont)
	    end
    end;
		
begin
//    decodeFont ('5x8.pcf')
    decodeFont (ParamStr (1))
end.
