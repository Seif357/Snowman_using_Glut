#include <windows.h>
#include <GL/glut.h>
#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>
#include <random>


// --- Animation & navigation state ---
static float armAnimAngle = 0;
static float armAnimPhase = 0.0f;
float snowmanX = 0.0f, snowmanZ = 0.0f;
float headingDeg = 0.0f; // y-axis, 0 = forward along -Z
float moveSpeed = 2.5f;  // units/sec
float rotSpeed = 90.0f;  // deg/sec

// Camera orbit/zoom parameters

static float angleX = 15.0f, angleY = 25.0f;
static float scaleFactor = 1.0f;

// Ground tile parameters
float groundTileSize = 25.0f;
int groundRepeat = 10;  // Ensures a big local patch: 7x7=49 tiles

float footstepPhase = 0.0f;
static bool LeftDown = false;
static float clickMouseX = 0.0f, clickMouseY = 0.0f;
static float angleXAtClick = 0.0f, angleYAtClick = 0.0f;
GLUquadric* quad = nullptr;

// Controls
bool keyW = false, keyS = false, keyA = false, keyD = false;
bool keyH = false;

// --- Sword slash fields
bool swordSlashing = false;
float swordSlashTimer = 0.0f;
const float swordSlashDuration = 0.35f; // seconds per slash
const float swordSlashMaxAngle = 100.0f; // degrees

// --- Footstep particle system ---
struct Particle {
    float x, y, z;
    float age, life;
};
static std::vector<Particle> particles;
const float footTrackX = 0.45f;

///////////////// ENVIRONMENT
struct Tree { float x, z, h, r; };
struct IceBlock { float x, z, s; };
std::vector<Tree> trees;
std::vector<IceBlock> iceblocks;

void generateEnvironment() {
    trees.clear();
    iceblocks.clear();
    std::mt19937 rng(9047);
    std::uniform_real_distribution<float> dist(-45.0f, 45.0f);
    std::uniform_real_distribution<float> rad(1.6f, 3.7f);
    std::uniform_real_distribution<float> hgt(2.6f, 5.5f);
    for (int i = 0; i < 38; ++i) {
        float x = dist(rng), z = dist(rng);
        if (std::sqrt(x * x + z * z) < 7.5f) continue; // keep open clearing
        Tree t; t.x = x; t.z = z; t.h = hgt(rng); t.r = rad(rng);
        trees.push_back(t);
    }
    std::uniform_real_distribution<float> bs(2.0f, 3.2f);
    for (int i = 0; i < 12; ++i) {
        float x = dist(rng), z = dist(rng);
        if (std::sqrt(x * x + z * z) < 8.5f) continue;
        IceBlock b; b.x = x; b.z = z; b.s = bs(rng);
        iceblocks.push_back(b);
    }
}

void drawPineTree(float h, float r) {
    // Brown trunk
    glColor3f(0.33f, 0.20f, 0.12f);
    glPushMatrix();
    glTranslatef(0, h * 0.15f, 0);
    glRotatef(270, 1, 0, 0);
    gluCylinder(quad, r * 0.20, r * 0.12, h * 0.3f, 8, 2);
    // Green cone
    glColor3f(0.19f, 0.41f, 0.1f); // deep pine
    glTranslatef(0, h * 0.1f, 1); // Move to top of trunk
    glRotatef(360, 1, 0, 0);
    glutSolidCone(r, h * 0.78, 16, 3);
    glPopMatrix();
}


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
    case 'h': keyH = true; break;

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
    case 'h': keyH = false; break;

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
    // Mouse wheel zoom (most freeglut/GLUT implementations)
    if (state == GLUT_DOWN) {
        if (button == 3) scaleFactor *= 0.9f;
        if (button == 4) scaleFactor *= 1.1f;
    }
    glutPostRedisplay();
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


void idle()
{
    static float lastTime = 0;
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float delta = time - lastTime;
    lastTime = time;

    // A/D turn snowman's body (not the camera!)
    if (keyA) headingDeg += rotSpeed * delta;
    if (keyD) headingDeg -= rotSpeed * delta;

    // Movement: move in SNOWMAN's heading
    bool moving = false;
    float walkDir = 0.0f;
    if (keyS) { walkDir += 1.0f; moving = true; }
    if (keyW) { walkDir -= 1.0f; moving = true; }
    if (walkDir != 0.0f) {
        float rad = -headingDeg * 3.1415926f / 180.0f;
        snowmanX += sinf(rad) * moveSpeed * delta * walkDir;
        snowmanZ += -cosf(rad) * moveSpeed * delta * walkDir;
        armAnimPhase += delta * 4.0f;
        footstepPhase += delta * 2.9f;
    }

    armAnimAngle = 28.0f * sinf(armAnimPhase);

    // Sword slash
    if (keyH && !swordSlashing) {
        swordSlashing = true;
        swordSlashTimer = 0.0f;
    }
    if (swordSlashing) {
        swordSlashTimer += delta;
        if (swordSlashTimer >= swordSlashDuration) {
            swordSlashing = false;
            swordSlashTimer = 0.0f;
        }
    }

    static float phaseRef = 0.0f;
    static bool lastFootLeft = false;
    float footSin = sinf(footstepPhase * 3.1415f);
    if (moving && footSin > 0.45f && phaseRef <= 0.45f) {
        Particle p;
        lastFootLeft = !lastFootLeft;
        float rad = headingDeg * 3.1415926f / 180.0f;
        float side = lastFootLeft ? -footTrackX : footTrackX;
        p.x = snowmanX + cosf(rad) * side;
        p.y = 0.0f;
        p.z = snowmanZ + sinf(rad) * side;
        p.age = 0.0f;
        p.life = 0.84f + 0.12f * (rand() % 100) / 100.f;
        particles.push_back(p);
    }
    phaseRef = footSin;

    for (size_t i = 0; i < particles.size(); ) {
        particles[i].age += delta;
        particles[i].y += delta * 0.14f;

        if (particles[i].age > particles[i].life) {
            particles.erase(particles.begin() + i);
        }
        else {
            ++i;
        }
    }

    glutPostRedisplay();
}

void initGL()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.83f, 0.92f, 1.0f, 1.0f);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

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
        {0,0,0},
        {0.07f,0.26f,0.26f},
        {0.23f,0.98f,0.91f},
        {0.45f,0.32f,0.11f},
        {0.1608f, 0.7725f, 0.6588f}

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
    glutWireCube(size + 0.001f);

    glLineWidth(1.0f);
    glPopMatrix();
}

// Carrot
void drawCarrotNose(float length, float radius)
{
    glColor3f(1.0f, 0.55f, 0.1f);

    glPushMatrix();
    gluCylinder(quad, radius, 0.0, length, 20, 3);
    glPopMatrix();
}

// Branch hand
void drawBranchHand(float baseLen, float baseRad)
{
    glColor3f(0.45f, 0.29f, 0.1f);
    glPushMatrix();
    gluCylinder(quad, baseRad, baseRad * 0.8, baseLen, 8, 2);
    glPushMatrix();
    glTranslatef(0, 0, baseLen * 0.75f);
    glRotatef(-40, 1, 0, 0);
    gluCylinder(quad, baseRad * 0.3f, baseRad * 0.2f, baseLen * 0.25f, 6, 2);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, 0, baseLen * 0.55f);
    glRotatef(45, 1, 0, 0);
    gluCylinder(quad, baseRad * 0.2f, 0.08f * baseRad, baseLen * 0.22f, 4, 2);
    glPopMatrix();
    glPopMatrix();
}


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

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Camera: orbit (angleX/Y, mouse) ---
    float camY = 4.0f;
    float camDist = 9.0f * scaleFactor;
    float cameraOrbitYaw = angleY;
    float camOrbitRad = cameraOrbitYaw * 3.1415926f / 180.0f;
    float camPitchRad = angleX * 3.1415926f / 180.0f;
    float camX = snowmanX - sinf(camOrbitRad) * cosf(camPitchRad) * camDist;
    float camZ = snowmanZ + cosf(camOrbitRad) * cosf(camPitchRad) * camDist;
    float camH = camY + sinf(camPitchRad) * camDist;

    glLoadIdentity();
    gluLookAt(
        camX, camH, camZ,
        snowmanX, camY, snowmanZ,
        0, 1, 0);

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

    // --- Draw trees & iceblocks
    for (const Tree& t : trees) {
        glPushMatrix();
        glTranslatef(t.x, 0, t.z);
        drawPineTree(t.h, t.r);
        glPopMatrix();
    }
    for (const IceBlock& b : iceblocks) {
        glPushMatrix();
        glColor3f(0.63f, 0.78f, 0.98f);
        glTranslatef(b.x, b.s / 2.f, b.z);
        glutSolidCube(b.s);
        glColor3f(0.7f, 0.85f, 1.0f);
        glutWireCube(b.s * 1.01f);
        glPopMatrix();
    }


    for (const Particle& p : particles) {
        float alpha = 1.0f - (p.age / p.life);
        glPushMatrix();
        glTranslatef(p.x, p.y + 0.02f, p.z);
        glDisable(GL_LIGHTING);
        glColor4f(0.96f, 0.95f, 0.91f, 0.38f * alpha);

        glutSolidSphere(0.12f * alpha, 8, 8);
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    // --- Sno
    glPushMatrix();
    glTranslatef(snowmanX, 0.0f, snowmanZ);
    glRotatef(headingDeg, 0, 1, 0);

    float baseSize = 2.0f;
    float bodySize = 1.5f;
    float headSize = 1.1f;

    glPushMatrix();
    glTranslatef(0, baseSize / 2, 0);
    drawSnowCube(baseSize);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0, baseSize + bodySize / 2 - 0.12f, 0);
    drawSnowCube(bodySize);
    glPopMatrix();
    float headY = baseSize + bodySize - 0.24f + headSize / 2;
    glPushMatrix();
    glTranslatef(0, headY, 0);
    drawSnowCube(headSize);
    glPopMatrix();

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

    float swordExtra = 0.0f;
    if (swordSlashing) {
        float t = swordSlashTimer / swordSlashDuration;
        float curve = std::sin(t * 3.14159f);
        swordExtra = swordSlashMaxAngle * curve;
    }
    glRotatef(-swordExtra, 0, 1, 0);
    drawMinecraftDiamondSword(0.14f);
    glPopMatrix();

    drawBranchHand(1.25f, 0.09f);
    glPopMatrix();

    float brimHeight = headSize * 0.07f;
    float topHeight = headSize * 0.62f;
    float brimY = headY + headSize / 2 + 0.01f;

    glPushMatrix();
    glColor3f(0.07f, 0.07f, 0.07f);
    glTranslatef(0, brimY, 0);
    glRotatef(-90, 1, 0, 0);
    solidCylinder(quad, headSize * 0.56f, headSize * 0.56f, brimHeight, 30, 3);
    glPopMatrix();
    // Top of Hat
    glPushMatrix();
    glColor3f(0.07f, 0.07f, 0.07f);
    glTranslatef(0, brimY + brimHeight, 0);
    glRotatef(-90, 1, 0, 0);
    solidCylinder(quad, headSize * 0.32f, headSize * 0.32f, topHeight, 30, 6);
    glPopMatrix();

    glPopMatrix(); // End of snowman


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
    glutCreateWindow("Minecraft Snow Man");


    initGL();
    generateEnvironment();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(special);
    glutMouseFunc(mouseButton);
    glutMotionFunc(motionWithButton);
    glutIdleFunc(idle);
    glutMainLoop();
    // Optional: if (quad) gluDeleteQuadric(quad);

    return 0;
}