#include <windows.h>      // Must come before <GL/glut.h> on Windows
#include <GL/glut.h>      // FreeGLUT/OpenGL Utility Toolkit
#include <algorithm>  // at top


static float angleX = 0.0f;
static float angleY = 0.0f;
// Initialize OpenGL state
void initGL()
{
    glEnable(GL_DEPTH_TEST);                // Enable depth buffering
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);    // Dark grey background
}

// Track cursor without pressing any buttons
void passiveMotion(int x, int y)
{
    // Map x,y to rotation (just an example)
    angleY = (x - 320) * 0.1f;   // assuming 640×480 window
    angleX = (y - 240) * 0.1f;
    glutPostRedisplay();
}
// Handle special keys (arrows, F-keys…)
void special(int key, int x, int y)
{
    const float delta = 5.0f;
    switch (key) {
    case GLUT_KEY_LEFT:
        angleY -= delta;   break;
    case GLUT_KEY_RIGHT:
        angleY += delta;   break;
    case GLUT_KEY_UP:
        angleX -= delta;   break;
    case GLUT_KEY_DOWN:
        angleX += delta;   break;
    default:
        break;
    }
    glutPostRedisplay();
}
// Handle “normal” key presses (ASCII)
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27:             // ESC
        exit(0);         // Quit on Esc
        break;
    case 'z':            // speed up rotation
        glScalef(2, 2, 2);
        break;
    case 'x':            // slow down
        glScalef(0.5, 0.5, 0.5);
        break;
    default:
        break;
    }
    glutPostRedisplay();
}

// Draw a cube centered at the origin, each face a different color
void biggestCube()
{
    glBegin(GL_QUADS);
    // Top face
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glVertex3f(-1, 1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);
    // Bottom face
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, -1, -1);
    glVertex3f(-1, -1, -1);
    // Front face
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);

    // Back face
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glVertex3f(-1, -1, -1);
    glVertex3f(1, -1, -1);
    glVertex3f(1, 1, -1);
    glVertex3f(-1, 1, -1);
    // Right face 
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glVertex3f(1, -1, -1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(1, 1, -1);
    // Left face
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glVertex3f(-1, 1, -1);
    glVertex3f(-1, 1, 1);
    glVertex3f(-1, -1, 1);
    glVertex3f(-1, -1, -1);
    glEnd();
}
// Display callback
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0, 0, -10);              // Move cube into view
    glRotatef(angleX, 1, 0, 0);
    glRotatef(angleY, 0, 1, 0);
    glPushMatrix();                    // save current transform
    biggestCube();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f); // lift it so its base sits on the top face
    glScalef(0.8f, 0.8f, 0.8f);     // make it smaller than the bottom cube
    biggestCube();
    glPopMatrix();
    glutSwapBuffers();
}

// Window reshape callback
void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / h, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
}


int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Simple Rotating Cube");

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);  // ASCII key events :contentReference[oaicite:0]{index=0}
    glutSpecialFunc(special);    // Non-ASCII key events :contentReference[oaicite:1]{index=1}
    glutPassiveMotionFunc(passiveMotion);  // Cursor move events :contentReference[oaicite:3]{index=3}

    glutMainLoop();
    return 0;
}
