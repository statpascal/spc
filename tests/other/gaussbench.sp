// gcc -O3 -fPIC -shared -o gaussbench.so gaussbench.cpp
// LD_PRELOAD=./gaussbench.so ../../obj/sp gaussbench.sp

program gaussbench;

type
    timespec = record
        tv_sec, tv_nsec: int64
    end;

function clock_gettime (clockid: integer; var ts: timespec): integer; external;

function clock_ms: int64;
    const
	clock_realtime = 0;
    var
	ts: timespec;
	res: int64;

    begin
	if clock_gettime (clock_realtime, ts) <> 0 then
	    begin
		writeln ('Fatal: Cannot get time');
		halt
	    end;
        clock_ms := 1000 * ts.tv_sec + ts.tv_nsec div (1000 * 1000)
    end;

function gaussden_sp (x, mu, sigma: real): real;
    const
	Sqrt2Pi = 2.5066282533;
    var
	xs: real;
    begin
	xs := (x - mu) / sigma;
	if abs (xs) > 37.0 then
	    gaussden_sp := 0.0
	else
  	    gaussden_sp := exp (-0.5 * xs * xs) / (Sqrt2Pi * sigma)
    end;

function gaussden_c (x, mu, sigma: real): real; external 'gaussbench.so';

type
    gaussdenfunc = function (x, mu, sigma: real): real;

procedure bench_gauss (msg: string; f: gaussdenfunc);
    const
        x = 1.0;
        mu = 0.5;
        sigma = 1.5;
        count = 100 * 1000 * 1000;

    var
	t0, t1: int64;
	i: 1..count;
        y: real;
    begin
	t0 := clock_ms;
	for i := 1 to count do
	    y := f (x, mu, sigma);
	t1 := clock_ms;
	writeln (msg, ': ', t1 - t0, ' ms')
    end;

begin
    bench_gauss ('SP ', gaussden_sp);
    bench_gauss ('C  ', gaussden_c)
end.
