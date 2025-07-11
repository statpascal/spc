uses tex, cfuncs;

function Cosinus(x: real): real;
    begin
	Cosinus := cos (x)
    end;

begin
    TeXOpen ('test1.tex');
    TeXBeginPic (-1, -2, 6.5, 2, 1, 2);
    TeXSetResolution (50, 100);
    TeXAxis (-0.8, 6, 1.5, -1.5, 1.5, 0.5, true, true);
    TeXPlot (Cosinus, -0.8, 6, -1.5, 1.5, 100);
    TeXEndPic;
    TeXClose;

    system ('pdflatex test');
    system ('evince test.pdf')
end.
