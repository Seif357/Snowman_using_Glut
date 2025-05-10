#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <ctime>

static float armAnimAngle = 0;

// Camera and transform
static float angleX = 15.0f, angleY = 25.0f;
static float scaleFactor = 1.0f;
static bool LeftDown = false;
static float clickMouseX = 0.0f, clickMouseY = 0.0f;
static float angleXAtClick = 0.0f, angleYAtClick = 0.0f;

// GLU quadric object for cones/cylinders etc.
GLUquadric* quad = nullptr;

void idle()
{
    // Calculate time in seconds
    static float lastTime = 0;
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f; // [sec]
    float delta = time - lastTime;
    lastTime = time;

    // Swing speed/pi controls speed of swing
    float speed = 2.0f; // controls how fast arms swing
    armAnimAngle = 35.0f * sinf(time * speed); // amplitude in degrees

    glutPostRedisplay();
}

// Draw capped cylinder using GLU
void solidCylinder(GLUquadric* q, double base, double top, double height, int slices, int stacks)
{
    gluCylinder(q, base, top, height, slices, stacks);

    // Draw base cap
    glPushMatrix();
    glRotatef(180, 1, 0, 0);
    gluDisk(q, 0.0, base, slices, 1);
    glPopMatrix();

    // Draw top cap
    glPushMatrix();
    glTranslatef(0, 0, height);
    gluDisk(q, 0.0, top, slices, 1);
    glPopMatrix();
}

void drawMinecraftDiamondSword(float voxel = 0.13f, int thickness = 2, int holdX = 4, int holdY = 14, int holdZ = 0)
{
    // Copied from original Minecraft diamond sword sprite (vertical)
    const int w = 17, h = 17;
    const int sword[17][17] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,4,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,1,2,4,2,1},
        {0,0,0,0,0,0,0,0,0,0,0,1,2,4,2,1,0},
        {0,0,0,0,0,0,0,0,0,0,1,2,4,2,1,0,0},
        {0,0,0,0,0,0,0,0,0,1,2,4,2,1,0,0,0},
        {0,0,0,0,0,0,0,0,1,2,4,2,1,0,0,0,0},
        {0,0,1,1,0,0,0,1,2,4,2,1,0,0,0,0,0},
        {0,0,1,4,1,0,1,2,4,2,1,0,0,0,0,0,0},
        {0,0,0,1,4,1,2,4,2,1,0,0,0,0,0,0,0},
        {0,0,0,1,4,1,4,2,1,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,4,1,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,3,3,1,4,4,1,0,0,0,0,0,0,0,0},
        {0,0,3,3,3,0,1,1,4,1,0,0,0,0,0,0,0},
        {1,1,3,3,0,0,0,0,1,1,0,0,0,0,0,0,0},
        {1,4,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},

    };


    // Colors: 0=empty, 1=edge, 2=diamond blade, 3=brown handle
    float colors[5][3] = {
        {0,0,0},                   // Not drawn
        {0.07f,0.26f,0.26f},       // Teal-black border
        {0.23f,0.98f,0.91f},       // Diamond blue
        {0.45f,0.32f,0.11f},        // Brown (handle)
        {0.1608f, 0.7725f, 0.6588f} // diamond half
    };

    // Center the sword at (0,0,0)
    glPushMatrix();
    glTranslatef(-(w / 2.0f) * voxel, -(h / 2.0f) * voxel, -thickness / 2.0f * voxel);

    float px = (holdX - w / 2.0f) * voxel;
    float py = (holdY - h / 2.0f) * voxel;
    float pz = (holdZ - thickness / 2.0f) * voxel;
    glTranslatef(-px, -py, -pz);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int c = sword[y][x];
            if (c == 0) continue;
            for (int z = 0; z < thickness; ++z) {
                glPushMatrix();
                glColor3fv(colors[c]);
                glTranslatef(x * voxel, y * voxel, z * voxel);
                glutSolidCube(voxel * 0.98f);
                glPopMatrix();
            }
        }
    }
    glPopMatrix();
}

// ...the rest of your code...
// Set up sky background
void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.65f, 0.85f, 1.0f, 1.0f); // sky blue
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    // Lighting setup
    GLfloat ambient[] = { 0.4, 0.4, 0.4, 1.0 };
    GLfloat diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
    GLfloat pos[] = { 40.0, 80.0, 13.0, 1.0 };
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_LIGHTING);
    quad = gluNewQuadric();
}

// Interaction handlers
void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            LeftDown = true;
            clickMouseX = (float)x;
            clickMouseY = (float)y;
            angleXAtClick = angleX;
            angleYAtClick = angleY;
        }
        else {
            LeftDown = false;
        }
    }
}
void motionWithButton(int x, int y)
{
    if (!LeftDown) return;
    float sensitivity = 0.3f;
    float deltaMouseX = (float)x - clickMouseX;
    float deltaMouseY = (float)y - clickMouseY;
    angleX = angleXAtClick + deltaMouseY * sensitivity;
    angleY = angleYAtClick + deltaMouseX * sensitivity;
    glutPostRedisplay();
}
void special(int key, int x, int y)
{
    const float delta = 5.0f;
    switch (key) {
    case GLUT_KEY_LEFT:  angleY -= delta; break;
    case GLUT_KEY_RIGHT: angleY += delta; break;
    case GLUT_KEY_UP:    angleX -= delta; break;
    case GLUT_KEY_DOWN:  angleX += delta; break;
    default: break;
    }
    glutPostRedisplay();
}
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27: exit(0); break;
    case 'z': scaleFactor *= 1.1f; break;
    case 'x': scaleFactor *= 0.9f; break;
    default: break;
    }
    glutPostRedisplay();
}

// Draw a white cube with gray wireframe/edges
void drawSnowCube(float size)
{
    glPushMatrix();
    // White faces
    glColor3f(1, 1, 1);
    glutSolidCube(size);
    // Gray edges overlay
    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(3.0f);
    glutWireCube(size + 0.001f); // epsilon bigger to avoid Z-fight
    glLineWidth(1.0f);
    glPopMatrix();
}

// Draw the carrot nose facing +z
void drawCarrotNose(float length, float radius)
{
    glColor3f(1.0f, 0.55f, 0.1f); // carrot orange
    glPushMatrix();
    // No rotation needed!
    gluCylinder(quad, radius, 0.0, length, 20, 3);
    glPopMatrix();
}

// Draw a single branch as a set of brown cylinders
void drawBranchHand(float baseLen, float baseRad)
{
    glColor3f(0.45f, 0.29f, 0.1f); // wood brown
    // Main branch
    glPushMatrix();
    gluCylinder(quad, baseRad, baseRad * 0.8, baseLen, 8, 2);

    // Upper twig
    glPushMatrix();
    glTranslatef(0, 0, baseLen * 0.75f);
    glRotatef(-40, 1, 0, 0);
    gluCylinder(quad, baseRad * 0.3, baseRad * 0.2, baseLen * 0.25, 6, 2);
    glPopMatrix();

    // Lower twig
    glPushMatrix();
    glTranslatef(0, 0, baseLen * 0.55f);
    glRotatef(45, 1, 0, 0);
    gluCylinder(quad, baseRad * 0.2, 0.08 * baseRad, baseLen * 0.22, 4, 2);
    glPopMatrix();

    glPopMatrix();
}

// Draw ice field: large checkered blue/white plane
void drawIceField(float size, int strips)
{
    float half = size / 2.0f;
    float tile = size / strips;
    for (int x = 0; x < strips; ++x) {
        for (int z = 0; z < strips; ++z) {
            if ((x + z) % 2 == 0)
                glColor3f(0.87f, 0.93f, 0.97f); // icy white
            else
                glColor3f(0.66f, 0.88f, 1.0f); // pale blue
            float sx = -half + x * tile;
            float sz = -half + z * tile;
            glBegin(GL_QUADS);
            glNormal3f(0, 1, 0);
            glVertex3f(sx, 0, sz);
            glVertex3f(sx + tile, 0, sz);
            glVertex3f(sx + tile, 0, sz + tile);
            glVertex3f(sx, 0, sz + tile);
            glEnd();
        }
    }
}

// --- Draw Everything ---
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    // Camera
    glTranslatef(0, -1.1f, -12.0f);
    glRotatef(angleX, 1, 0, 0);
    glRotatef(angleY, 0, 1, 0);
    glScalef(scaleFactor, scaleFactor, scaleFactor);

    // Draw the ice biome
    glPushMatrix();
    glTranslatef(0, -2.1f, 0);
    drawIceField(13.0f, 18);
    glPopMatrix();

    // --- Snowman Body Cubes ---
    float baseSize = 2.0f;
    float bodySize = 1.5f;
    float headSize = 1.1f;

    // Bottom
    glPushMatrix();
    glTranslatef(0, baseSize / 2, 0);
    drawSnowCube(baseSize);
    glPopMatrix();

    // Torso
    glPushMatrix();
    glTranslatef(0, baseSize + bodySize / 2 - 0.12f, 0);
    drawSnowCube(bodySize);
    glPopMatrix();

    // Head
    glPushMatrix();
    float headY = baseSize + bodySize - 0.24f + headSize / 2;
    glTranslatef(0, headY, 0);
    drawSnowCube(headSize);
    glPopMatrix();

    // --- Eyes (black spheres) ---
    float eyeOffsetY = headY + headSize * 0.18f;
    float eyeOffsetZ = headSize / 2 + 0.01f;
    float eyeOffsetX = headSize * 0.21f;

    glPushMatrix();
    glColor3f(0, 0, 0);
    glTranslatef(-eyeOffsetX, eyeOffsetY, eyeOffsetZ);
    glutSolidSphere(0.08 * headSize, 15, 15);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0, 0, 0);
    glTranslatef(eyeOffsetX, eyeOffsetY, eyeOffsetZ);
    glutSolidSphere(0.08 * headSize, 15, 15);
    glPopMatrix();

    // --- Carrot Nose ---
    float noseLen = 0.43f;
    float noseRadius = 0.10f;
    glPushMatrix();
    glTranslatef(0, headY, headSize / 2);
    drawCarrotNose(noseLen, noseRadius);
    glPopMatrix();

    // Place this in your display() arm drawing area:

    float armY = baseSize + bodySize * 0.5f - 0.05f;
    float armLeftX = -(bodySize / 2 + 0.01f);
    float armRightX = (bodySize / 2 + 0.01f);
    float armZ = 0;

    float outwardAngle = 90; // adjust as you like

    // Left Arm: straight down, then outward, then animate
    glPushMatrix();
    glTranslatef(armLeftX, armY, armZ);
    glRotatef(90, 1, 0, 0);                // Down
    glRotatef(outwardAngle, -3, -3, -2);       // Outward (away from body)
    glRotatef(armAnimAngle, 1, 0, 0);       // Animate forward/back
    drawBranchHand(1.25f, 0.09f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(armRightX, armY, armZ);
    glRotatef(-90, 1, 0, 0);                 // Down
    glRotatef(-outwardAngle, -3, 3, 2);       // Outward
    glRotatef(-armAnimAngle, 1, 0, 0);       // Animate
    glScalef(1, 1, -1);
    // DRAW SWORD FIRST, so it's under the hand
    // Positioning sword at tip of branch, "Minecraft style tilt"
    glPushMatrix();
    glTranslatef(0, 0, 1.10f);           // Extend to tip of branch
    glRotatef(-40, 0, 0, 1);             // Classic MC sword angle
    drawMinecraftDiamondSword(0.14f);    // Slightly larger voxels for thickness
    glPopMatrix();
    drawBranchHand(1.25f, 0.09f);
    glPopMatrix();

    // Before drawing the hat
    float brimHeight = headSize * 0.07f;
    float topHeight = headSize * 0.62f;
    float brimY = headY + headSize / 2 + 0.01f; // base of the brim
    float topY = brimY + brimHeight;           // base of top at the top of the brim

	//Kanzen
    // Brim
    glPushMatrix();
    glColor3f(0.07f, 0.07f, 0.07f); // jet black
    glTranslatef(0, brimY, 0);
    glRotatef(-90, 1, 0, 0);
    solidCylinder(quad, headSize * 0.56f, headSize * 0.56f, brimHeight,
        30, 3);
    glPopMatrix();

    // Top
    glPushMatrix();
    glColor3f(0.07f, 0.07f, 0.07f);
    glTranslatef(0, topY, 0); // not headY... base of top is at topY
    glRotatef(-90, 1, 0, 0);
    solidCylinder(quad, headSize * 0.32f, headSize * 0.32f, topHeight,
        30, 6);
    glPopMatrix();

    glutSwapBuffers();
}

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
    glutInitWindowSize(900, 600);
    glutCreateWindow("Advanced OpenGL CUBE Snowman - Ice Biome");

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouseButton);
    glutMotionFunc(motionWithButton);
    glutIdleFunc(idle);
    glutMainLoop();
    // Never reached, but in real app:
    // if (quad) gluDeleteQuadric(quad);
    return 0;
}