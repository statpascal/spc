unit opengl;

(* Minimal interface to display the Utah teapot *)

interface

const
    libGL = 'libGL.so';
    libglut = 'libglut.so.3';

    GL_PROJECTION = $1701;
    GL_COLOR_BUFFER_BIT = $4000;
    GL_MODELVIEW = $1700;

procedure glMatrixMode (mode: integer); external libGL;
procedure glLoadIdentity; external libGL;
procedure glFrustum (x0, x1, y0, y1, z0, z1: double); external libGL;
procedure glClear (mode: integer); external libGL;
procedure glTranslatef (x, y, z: single); external libGL;
procedure glRotatef (angle, x, y, z: single); external libGL;
procedure glFlush; external libGL;

procedure glutInit (var argc: integer; argv: pointer); external libglut;
procedure glutWireTeapot (size: double); external libglut;
procedure glutCreateWindow (title: pointer); external libglut;
procedure glutDisplayFunc (callback: procedure); external libglut;
procedure glutMainLoop; external libglut;

implementation

end.
