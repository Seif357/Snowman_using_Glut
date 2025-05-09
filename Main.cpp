#include<iostream>
#include<windows.h>      // Must come before <GL/glut.h> on Windows
#include<GL/glut.h>      // FreeGLUT/OpenGL Utility Toolkit
#include<algorithm>  // at top
#include<GLFW/glfw3.h> 

static float angleX = 0.0f;
static float angleY = 0.0f;
static float scaleFactor = 1.0f;
static bool LeftDown = false;
// Initialize OpenGL state
// New static variables for improved mouse dragging
static float clickMouseX = 0.0f;    // Mouse X position when left button was pressed
static float clickMouseY = 0.0f;    // Mouse Y position when left button was pressed
static float angleXAtClick = 0.0f;  // AngleX at the moment of left button press
static float angleYAtClick = 0.0f;  // AngleY at the moment of left button press
void initGL()
{
    glEnable(GL_DEPTH_TEST);                // Enable depth buffering
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);    // Dark grey background
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);   // Offset fill polys “into” the screen                  :contentReference[oaicite:0]{index=0}

}
// Mouse button callback
void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            LeftDown = true;
            // Record the initial mouse position and current angles when the button is pressed
            clickMouseX = static_cast<float>(x);
            clickMouseY = static_cast<float>(y);
            angleXAtClick = angleX;
            angleYAtClick = angleY;
        }
        else { // state == GLUT_UP
            LeftDown = false;
            // angleX and angleY now hold the orientation after the drag,
            // which will be the starting point for the next drag if angleXAtClick/angleYAtClick are updated.
        }
    }
}

// Mouse motion callback (called when a button is pressed and mouse moves)
void motionWithButton(int x, int y)
{
    if (!LeftDown)
        return;

    // Sensitivity factor for rotation speed, can be adjusted
    float sensitivity = 0.1f;

    // Calculate the difference in mouse position from where the button was pressed
    float deltaMouseX = static_cast<float>(x) - clickMouseX;
    float deltaMouseY = static_cast<float>(y) - clickMouseY;

    // Update angles: new angle = angle at click time + rotation due to mouse drag
    // Mouse Y movement typically controls rotation around X-axis (pitch)
    // Mouse X movement typically controls rotation around Y-axis (yaw)
    angleX = angleXAtClick + deltaMouseY * sensitivity;
    angleY = angleYAtClick + deltaMouseX * sensitivity;

    glutPostRedisplay(); // Request a redraw
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
    case 27: // ESC
        exit(0);
    case 'z': // increase scale
        scaleFactor *= 1.1f;  // 10% bigger
        break;
    case 'x': // decrease scale
        scaleFactor *= 0.9f;  // 10% smaller
        break;
    default:
        break;
    }
    glutPostRedisplay();
}

// Draw a cube centered at the origin, each face a different color
void display()
{
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0, 0, -10);              // Move cube into view
    glRotatef(angleX, 1, 0, 0);
    glRotatef(angleY, 0, 1, 0);
    glPushMatrix();                    // save current transform
    glColor3f(0.8431372549019608f, 0.9215686274509804f, 0.9568627450980392f);
    glScalef(scaleFactor, scaleFactor, scaleFactor);
    glutSolidCube(2.0);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 1.5f * scaleFactor, 0);   // lift relative to scaled base
    glScalef(scaleFactor * 0.8f, scaleFactor * 0.8f, scaleFactor * 0.8f);
    glutSolidCube(2.0);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, (1.5f + 1.5f * 0.8f) * scaleFactor, 0);
    glScalef(scaleFactor * 0.7f, scaleFactor * 0.7f, scaleFactor * 0.7f);
    glutSolidCube(2.0);
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
    glutMouseFunc(mouseButton);
    glutMotionFunc(motionWithButton);

    glutMainLoop();
    return 0;
}
