#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <ctime>
#include <vector>

// --- Animation & navigation state ---
static float armAnimAngle = 0;
static float armAnimPhase = 0.0f;

float snowmanX = 0.0f, snowmanZ = 0.0f;
float headingDeg = 0.0f; // y-axis, 0 = forward along -Z

// Movement speed/rot
float moveSpeed = 2.5f;  // units/sec
float rotSpeed = 90.0f;  // deg/sec

// Ground tile parameters
float groundTileSize = 7.0f;
int groundRepeat = 7;  // (Ensures a big local patch: 7x7=49 tiles)

// Step animation phase (for footsteps)
float footstepPhase = 0.0f;

// Camera and transform
static float angleX = 15.0f, angleY = 25.0f;
static float scaleFactor = 1.0f;
static bool LeftDown = false;
static float clickMouseX = 0.0f, clickMouseY = 0.0f;
static float angleXAtClick = 0.0f, angleYAtClick = 0.0f;

// GLU quadric object for cones/cylinders etc.
GLUquadric* quad = nullptr;

// --- Key controls
bool keyW = false, keyS = false, keyA = false, keyD = false;

// --- Footstep particle system ---
struct Particle {
    float x, y, z;
    float age, life;
};
static std::vector<Particle> particles;

// For alternating left/right tracks
const float footTrackX = 0.45f;   // Left/right side offset (body width)
const float footTrackZ = -1.36f;  // Not used in world coords anymore

// --- Keyboard handlers
void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 27: exit(0); break;
    case 'z': scaleFactor *= 1.1f; break;
    case 'x': scaleFactor *= 0.9f; break;
    case 'w': keyW = true; break;
    case 's': keyS = true; break;
    case 'a': keyA = true; break;
    case 'd': keyD = true; break;
    }
    glutPostRedisplay();
}
void keyboardUp(unsigned char key, int x, int y)
{
    switch (key) {
    case 'w': keyW = false; break;
    case 's': keyS = false; break;
    case 'a': keyA = false; break;
    case 'd': keyD = false; break;
    }
}
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

// --- Animation, movement, and particles ---
void idle()
{
    static float lastTime = 0;
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float delta = time - lastTime;
    lastTime = time;

    // --- Rotation (A/D keys): ---
    if (keyA) headingDeg += rotSpeed * delta;
    if (keyD) headingDeg -= rotSpeed * delta;

    // --- Movement (W/S keys): ---
    bool moving = false;
    float walkDir = 0.0f;
    if (keyW) { walkDir += 1.0f; moving = true; }
    if (keyS) { walkDir -= 1.0f; moving = true; }
    if (walkDir != 0.0f) {
        float rad = headingDeg * 3.1415926f / 180.0f;
        snowmanX += sinf(rad) * moveSpeed * delta * walkDir;
        snowmanZ += -cosf(rad) * moveSpeed * delta * walkDir; // -cos for OpenGL -Z fwd
        armAnimPhase += delta * 4.0f; // Animate only when moving
        footstepPhase += delta * 2.9f;
    }

    armAnimAngle = 28.0f * sinf(armAnimPhase); // 80% original amplitude

    // --- Particle emission: alternate left/right on each footstep (world coords) ---
    static float phaseRef = 0.0f;
    static bool lastFootLeft = false;
    float footSin = sinf(footstepPhase * 3.1415f);
    // Each time the step sine crosses zero from negative: emit
    if (moving && footSin > 0.45f && phaseRef <= 0.45f) {
        // One footprint, sync with walk cycle
        Particle p;
        lastFootLeft = !lastFootLeft;
        float rad = headingDeg * 3.1415926f / 180.0f;
        float side = lastFootLeft ? -footTrackX : footTrackX;
        // Left/right offset perpendicular to heading
        p.x = snowmanX + cosf(rad) * side;
        p.y = 0.0f;
        p.z = snowmanZ + sinf(rad) * side;
        p.age = 0.0f;
        p.life = 0.84f + 0.12f * (rand() % 100) / 100.f;
        particles.push_back(p);
    }
    phaseRef = footSin;

    // --- Update + prune particles ---
    for (size_t i = 0; i < particles.size(); ) {
        particles[i].age += delta;
        particles[i].y += delta * 0.14f; // Float up gently
        if (particles[i].age > particles[i].life) {
            particles.erase(particles.begin() + i);
        }
        else {
            ++i;
        }
    }

    glutPostRedisplay();
}

// --- GL setup ---
void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.83f, 0.92f, 1.0f, 1.0f); // snow sky light blue
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
    // Lighting setup
    GLfloat ambient[] = { 0.5, 0.5, 0.58, 1.0 };
    GLfloat diffuse[] = { 0.9, 0.9, 1.0, 1.0 };
    GLfloat specular[] = { 0.2,0.2,0.6,1.0 };
    GLfloat pos[] = { 30.0, 80.0, 33.0, 1.0 };
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_LIGHTING);
    quad = gluNewQuadric();
}

// Cylinder/Disk for hat etc.
void solidCylinder(GLUquadric* q, double base, double top, double height, int slices, int stacks)
{
    gluCylinder(q, base, top, height, slices, stacks);
    glPushMatrix();
    glRotatef(180, 1, 0, 0);
    gluDisk(q, 0.0, base, slices, 1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, height);
    gluDisk(q, 0.0, top, slices, 1);
    glPopMatrix();
}

// --- The sword (unchanged)
void drawMinecraftDiamondSword(float voxel = 0.13f, int thickness = 2, int holdX = 4, int holdY = 14, int holdZ = 0)
{
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
    float colors[5][3] = {
        {0,0,0},                   // Not drawn
        {0.07f,0.26f,0.26f},       // Teal-black border
        {0.23f,0.98f,0.91f},       // Diamond blue
        {0.45f,0.32f,0.11f},        // Brown (handle)
        {0.1608f, 0.7725f, 0.6588f} // diamond half
    };
    glPushMatrix();
    glTranslatef(-(w / 2.0f) * voxel, -(h / 2.0f) * voxel, -thickness / 2.0f * voxel);
    float px = (holdX - w / 2.0f) * voxel;
    float py = (holdY - h / 2.0f) * voxel;
    float pz = (holdZ - thickness / 2.0f) * voxel;
    glTranslatef(-px, -py, -pz);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
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
    glPopMatrix();
}

// White cube with edge
void drawSnowCube(float size)
{
    glPushMatrix();
    glColor3f(1, 1, 1);
    glutSolidCube(size);
    glColor3f(0.86f, 0.95f, 0.98f);
    glLineWidth(3.0f);
    glutWireCube(size + 0.001f); // epsilon bigger to avoid Z-fight
    glLineWidth(1.0f);
    glPopMatrix();
}

// Carrot
void drawCarrotNose(float length, float radius)
{
    glColor3f(1.0f, 0.55f, 0.1f); // carrot orange
    glPushMatrix();
    gluCylinder(quad, radius, 0.0, length, 20, 3);
    glPopMatrix();
}

// Branch hand
void drawBranchHand(float baseLen, float baseRad)
{
    glColor3f(0.45f, 0.29f, 0.1f); // wood brown
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
    gluCylinder(quad, baseRad * 0.2, 0.08f * baseRad, baseLen * 0.22f, 4, 2);
    glPopMatrix();

    glPopMatrix();
}

// Snowy ground: subtle blue/white random "snow"
void drawIceField(float size, int strips)
{
    GLfloat snow_amb[] = { 0.86f,0.92f,1.0f,1.0f };
    GLfloat snow_diff[] = { 0.96f,0.98f,1.0f,1.0f };
    GLfloat snow_spec[] = { 0.12f,0.19f,0.30f,1.0f };
    float shininess = 24.0f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, snow_amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, snow_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, snow_spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    float half = size / 2.0f;
    float tile = size / strips;
    for (int x = 0; x < strips; ++x) {
        for (int z = 0; z < strips; ++z) {
            float w = 0.97f + 0.04f * (rand() % 100) / 100.f;
            float b = 0.97f + 0.03f * (rand() % 100) / 100.f;
            glColor3f(w, w, b);
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

    // --- Endless ground tiles ---
    float nearTileX = groundTileSize * std::round(snowmanX / groundTileSize);
    float nearTileZ = groundTileSize * std::round(snowmanZ / groundTileSize);
    for (int gx = -groundRepeat / 2; gx <= groundRepeat / 2; ++gx) {
        for (int gz = -groundRepeat / 2; gz <= groundRepeat / 2; ++gz) {
            glPushMatrix();
            glTranslatef(
                nearTileX + gx * groundTileSize,
                -0.02f,
                nearTileZ + gz * groundTileSize
            );
            drawIceField(groundTileSize, 32);
            glPopMatrix();
        }
    }

    // --- SNOW PARTICLES / TRACKS ---
    for (const Particle& p : particles) {
        float alpha = 1.0f - (p.age / p.life);
        glPushMatrix();
        glTranslatef(p.x, p.y + 0.02f, p.z);
        glDisable(GL_LIGHTING);
        glColor4f(0.91f, 0.97f, 1.0f, 0.38f * alpha);
        glutSolidSphere(0.12f * alpha, 8, 8);
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    // --- Draw snowman centered at (snowmanX, 0, snowmanZ), rotated by heading ---
    glPushMatrix();
    glTranslatef(snowmanX, 0.0f, snowmanZ);
    glRotatef(headingDeg, 0, 1, 0);

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

    // --- Eyes ---
    float eyeOffsetY = headY + headSize * 0.18f;
    float eyeOffsetZ = headSize / 2 + 0.01f;
    float eyeOffsetX = headSize * 0.21f;

    glPushMatrix();
    glColor3f(0, 0, 0);
    glTranslatef(-eyeOffsetX, eyeOffsetY, eyeOffsetZ);
    glutSolidSphere(0.08f * headSize, 15, 15);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0, 0, 0);
    glTranslatef(eyeOffsetX, eyeOffsetY, eyeOffsetZ);
    glutSolidSphere(0.08f * headSize, 15, 15);
    glPopMatrix();

    // --- Carrot Nose ---
    float noseLen = 0.43f;
    float noseRadius = 0.10f;
    glPushMatrix();
    glTranslatef(0, headY, headSize / 2);
    drawCarrotNose(noseLen, noseRadius);
    glPopMatrix();

    float armY = baseSize + bodySize * 0.5f - 0.05f;
    float armLeftX = -(bodySize / 2 + 0.01f);
    float armRightX = (bodySize / 2 + 0.01f);
    float armZ = 0;
    float outwardAngle = -90;

    // Left Arm
    glPushMatrix();
    glTranslatef(armLeftX, armY, armZ);
    glRotatef(180-outwardAngle, -1, 1, 0);        // Outward
    glRotatef(armAnimAngle, -1, 1, 0);           // Animate
    drawBranchHand(1.25f, 0.09f);
    glPopMatrix();

    // --- Right Arm + SWORD (flipped for "up") ---
    glPushMatrix();
    glTranslatef(armRightX, armY, armZ);
    glRotatef(outwardAngle,1 , 1, 0);         // Outward
    glRotatef(armAnimAngle, 1, 1, 0);          // Animate
    glScalef(1, 1, -1);                         // Mirror

    // Sword under branch
    glPushMatrix();
    glTranslatef(0, 0, 1.10f);
    glRotatef(-40, 0, 0, 1);
    glRotatef(270, 1, 0, 0);
    drawMinecraftDiamondSword(0.14f);
    glPopMatrix();
    drawBranchHand(1.25f, 0.09f);
    glPopMatrix();


    // --- Hat ---
    float brimHeight = headSize * 0.07f;
    float topHeight = headSize * 0.62f;
    float brimY = headY + headSize / 2 + 0.01f;
    float topY = brimY + brimHeight;

    // Brim
    glPushMatrix();
    glColor3f(0.07f, 0.07f, 0.07f);
    glTranslatef(0, brimY, 0);
    glRotatef(-90, 1, 0, 0);
    solidCylinder(quad, headSize * 0.56f, headSize * 0.56f, brimHeight, 30, 3);
    glPopMatrix();

    // Top
    glPushMatrix();
    glColor3f(0.07f, 0.07f, 0.07f);
    glTranslatef(0, brimY + brimHeight, 0);
    glRotatef(-90, 1, 0, 0);
    solidCylinder(quad, headSize * 0.32f, headSize * 0.32f, topHeight, 30, 6);
    glPopMatrix();

    glPopMatrix(); // End snowman matrix

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
    glutCreateWindow("Advanced OpenGL Minecraft-Style Snowman - WASD Endless World");

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutMouseFunc(mouseButton);
    glutMotionFunc(motionWithButton);
    glutIdleFunc(idle);
    glutMainLoop();
    // Never reached:
    // if (quad) gluDeleteQuadric(quad);
    return 0;
}