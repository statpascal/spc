program gtktest;

uses gtk3, cfuncs, cthreads;

var 
    count: integer;
    running: boolean;

function notifyUpdate (user_data: gpointer): boolean; forward;

function countThread (data: pointer): ptrint;
    var
	dummy: uint32;
    begin
	count := 0;
	repeat
	    inc (count);
	    gdk_threads_add_idle (addr (notifyUpdate), nil);
	    dummy := usleep (50 * 1000)
	until not running;
	countThread := 0
    end;

procedure windowClosed (sender: PGtkWidget; user_data: gpointer);
    begin
	writeln ('Window is closed - terminating application');
 	gtk_main_quit
    end;

function drawCallback (window: PGtkWidget; cr: PCairoT; data: gpointer): boolean;
    var 
	s: string;
    begin
	str (count, s);
	writeln ('Draw callback');
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_set_line_width (cr, 1);
	cairo_move_to (cr, 50, 50);
	cairo_select_font_face (cr, 'sans_serif', 0, 0);
	cairo_set_font_size (cr, 15);
	cairo_show_text (cr, 'Hello, world ' + s);
	cairo_stroke (cr);
	drawCallback := true
    end;

var
    argv: argvector;
    argc: integer;
    window, drawingArea: PGtkWidget;
    title: string;
    tid: TThreadId;

function notifyUpdate (user_data: gpointer): boolean;
    begin
	gtk_widget_queue_draw (window);
	notifyUpdate := false;
    end;

begin
    argc := 0;
    argv := nil;
    gtk_init (argc, argv);
    window := gtk_window_new (GTK_WINDOW_TOPLEVEL);
    title := 'GTK window from SP';
    gtk_window_set_title (window, addr (title [1]));
    g_signal_connect (window, 'destroy', addr (windowClosed), nil);

    drawingArea := gtk_drawing_area_new;
    gtk_container_add (window, drawingArea);
    gtk_widget_set_size_request (drawingArea, 500, 300);

    g_signal_connect (drawingArea, 'draw', addr (drawCallback), nil);

    gtk_widget_show_all (window);

    running := true;
    beginThread (countThread, nil, tid);
    gtk_main;
    writeln ('gtk_main returned');

    running := false;
    waitForThreadTerminate (tid, 0);
    writeln ('Thread terminated')
end.
