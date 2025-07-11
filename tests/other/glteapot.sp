program glteapot;

uses opengl;

var 
    argc: integer;

procedure display;
    begin
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity;
        glFrustum (-1, 1, -1, 1, 1, 5);
        
        glClear (GL_COLOR_BUFFER_BIT);
        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity;
        glTranslatef (0.0, 0.0, -3.0);
        glRotatef (45.0, 1, 1, 1);
        glutWireTeapot (1.2);
        glFlush
    end;

var
    s: array [1..7] of char;
    t: string;

begin
    argc := 0;
    glutInit (argc, nil);
    t := 'Teapot';
    move (t [1], s [1], 7);
    glutCreateWindow (addr (s [1]));
    glutDisplayFunc (display);
    glutMainLoop
end.
