program gtktest;

uses gtk3;

const
    n = 4;

procedure WindowClosed (Sender: PGtkWidget; user_data: gpointer); export;
    begin
	writeln ('Window is closed - terminating application');
 	gtk_main_quit
    end;

procedure ButtonClicked (Sender: PGtkWidget; user_data: gpointer); export;
    begin
	writeln ('Button clicked: ', char (user_data^))
    end;

var
    argv: argvector;
    argc: integer;
    window, button, grid: PGtkWidget;
    desc: array [1..n] of char;
    i: 1..n;

begin
    argc := 0;
    argv := nil;
    gtk_init (argc, argv);
    window := gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (window, 'GTK window from SP');

    g_signal_connect (window, 'destroy', addr (WindowClosed), nil);
    grid := gtk_grid_new;

    for i := 1 to n do
	begin
	    desc [i] := chr (ord ('0') + i);
    	    button := gtk_button_new_with_label ('Button ' + desc [i]);
            g_signal_connect (button, 'clicked', addr (ButtonClicked), gpointer (addr (desc [i])));
	    gtk_grid_attach (grid, button, i, 0, 1, 1)
	end;

    gtk_container_add (window, grid);

    gtk_widget_show_all (window);
    gtk_main;
    writeln ('gtk_main returned')
end.
