program sdltest;

uses sdl2;

const SampleRate = 44100;
      BufferSize = 2048;
      Amplitude = 20000;
      Frequency = 440;

procedure sdlCallback (userdata, stream: pointer; len: int32); export;
    const
	count: int64 = 0;
    var 
        i: integer;
        buffer: ^int16;
	offset: real;
    begin
	buffer := stream;
	for i := 0 to pred (len div 2) do
	    begin 
		offset := count / SampleRate;
		buffer [i] := round (Amplitude * sin (2.0 * Pi * Frequency * offset));
		inc (count)
	    end;
    end;

procedure runtest;
    var 
	desired, obtained: SDL_AudioSpec;
	dummy: int32;
    begin
	with desired do
	    begin
		freq := SampleRate;
		format := AUDIO_S16;
		channels := 1;
		samples := BufferSize;
		callback := addr (sdlCallback);
		userdata := nil
	    end;
	dummy := SDL_Init (SDL_INIT_AUDIO);
        dummy := SDL_OpenAudio (desired, obtained);
        SDL_PauseAudio (0);
	writeln ('Hit RETURN to stop sound...');
	readln;
	SDL_PauseAudio (1);
	SDL_CloseAudio
    end;

begin
    runtest
end.
