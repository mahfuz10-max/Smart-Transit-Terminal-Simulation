#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

// Old Windows/MinGW OpenGL 1.1 headers do not always expose this token.
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
#ifndef GLUT_MULTISAMPLE
#define GLUT_MULTISAMPLE 128
#endif
#ifndef GL_LIGHT3
#define GL_LIGHT3 0x4003
#endif
#ifndef GL_LIGHT4
#define GL_LIGHT4 0x4004
#endif
#ifndef GL_QUADRATIC_ATTENUATION
#define GL_QUADRATIC_ATTENUATION 0x1209
#endif

// ============================================================================
// NEXBUS Urban Hub - modern city bus terminal - OpenGL/GLUT
// CSE Computer Graphics project (beginner-friendly fixed-function OpenGL)
//
// Requirements demonstrated:
//  1) Complete 3D scene
//  2) Translation, rotation, scaling via keyboard/mouse
//  3) Complex objects (buses, terminal, canopies, windmill, lamps, trees)
//  4) Continuous rotating objects (windmill + ceiling fans)
//  5) Window/glass facade with external environment visible from interior view
//  6) Multiple lights + ambient/diffuse/specular materials
//  7) Object-coordinate and viewing-coordinate transformations
//  8) Texture mapping (procedural asphalt, brick, pavement, wood, metal, grass)
// ============================================================================

const float PI = 3.14159265358979323846f;

// LIGHT0 is directional. A shadow travels opposite this X/Z vector, so both
// setupLights() and drawSoftShadow() share these values instead of drifting
// apart through unrelated hard-coded offsets.
const float PRIMARY_LIGHT_X = -0.30f;
const float PRIMARY_LIGHT_Y =  1.00f;
const float PRIMARY_LIGHT_Z =  0.28f;

// ---------------------------- Window state -----------------------------------
int winW = 1280, winH = 760;

// ---------------------------- Camera state -----------------------------------
float camX = 26.0f, camY = 18.0f, camZ = 30.0f;
float yawDeg = -135.0f;
float pitchDeg = -22.0f;
int cameraPreset = 1;
float trafficSyncTime = 0.0f;

// ----------------------- Object transformation demo --------------------------
float objectTX = 0.0f, objectTY = 0.0f, objectTZ = 0.0f;
float objectRotY = 0.0f;
float objectScale = 1.0f;

// ----------------------------- Animation -------------------------------------
float windAngle = 0.0f;
float fanAngle = 0.0f;
float movingBusX = -24.0f;
float movingCarZ = 20.0f;
float busWheelRotation = 0.0f;
bool weatherRain = false;
void drawRainEffect();
void drawAnimatedLEDDisplay();
void drawAnimatedTrafficLights();
void drawMovingTraffic();
void drawFinalCinematicCamera();
void drawRainRoadReflection();
void drawSkyBackdrop();
void drawSkyCloudCluster(float x, float y, float z, float scale, float alpha);
void drawExtraRoadDetail();
void drawGrassTuft(float x, float z, float scale = 1.0f);
void drawLandscapePatch(float x1, float z1, float x2, float z2);
void drawMiniBus(float r = 0.84f, float g = 0.12f, float b = 0.10f,
                 bool liveEffects = true);
void drawMiniBusLights(float brake, float indicator);
void drawV37Branding();
void drawV37RouteSigns();
void drawAutoRickshaw(float x, float z, float rot = 0.0f, bool moving = false);
void drawCycleRickshaw(float x, float z, float rot = 0.0f, bool moving = false);
void drawDividerPlantDust();
void drawUtilityPole(float x, float z, float h = 6.2f);
void drawTaxi(float r = 0.96f, float g = 0.78f, float b = 0.10f);
void drawMotorcycle(float r = 0.12f, float g = 0.16f, float b = 0.18f);
void drawV26TrafficFlow();
void drawV26WalkingPassengers();
void drawV26BayBusAnimation();
void drawV27RoadSurfaceGlow();
void drawV27TerminalNightGlow();
void drawV27CityLightingAccent();
void drawSwayingTrees();
void drawV28WindowViewTraffic();
void drawV46GlassReflection();
void drawCleanInteriorView();
void drawRotatingRoofVent(float x, float z);
void drawDriverCabin();
void drawCityDynamicOverlays();
void drawParkedVehicleNightOverlays();
void drawHuman(float x, float z, float shirtR, float shirtG, float shirtB,
               bool walking = false, float walkPhase = 0.0f,
               float facingDeg = 0.0f, bool seated = false);
float rainOffset = 0.0f;
float ledPulse = 0.0f;
float busDoorAngle = 0.0f;
float rainReflectionPulse = 0.0f;
float roadWetness = 0.0f;
float trafficFlowX = -30.0f;
float trafficLightTimer = 0.0f;
int trafficLightState = 0;
float dayNightBlend = 0.0f;
float dayNightTarget = 0.0f;
float indicatorBlinkTimer = 0.0f;
float cloudDrift = 0.0f;
float birdDrift = 0.0f;      // V69: drives the flying-bird flock's looping flight path
float birdFlapPhase = 0.0f;  // V69: drives the flying-bird flock's wing flap
float taxiMotion = -36.0f;
float motorcycleMotion = 34.0f;
float trafficBrakePulse = 0.0f;
float indicatorPulse = 0.0f;
float crossingWalk = -5.0f;
float bayBusPhase = 0.0f;
bool animationOn = true;
bool busMotionOn = true;
bool nightMode = false;
bool showHUD = false;
bool showAxes = false;
bool cinematicTour = false;
int fpsCounter = 0;
int fpsValue = 0;
int fpsTimer = 0;
float cinematicTime = 0.0f;
const std::string TERMINAL_BRAND = "DHAKA CITY TERMINAL";

// ------------------------------ Mouse ----------------------------------------
int lastMouseX = 0, lastMouseY = 0;
bool leftDragging = false;
bool rightDragging = false;
bool middleDragging = false;

// ------------------------------ Textures -------------------------------------
GLuint texRoad = 0;
GLuint texBrick = 0;
GLuint texWood = 0;
GLuint texMetal = 0;
GLuint texPavement = 0;
GLuint texGrass = 0;

// Static display lists are built once after procedural textures are uploaded.
//  0: unlit ground/roads/details, 1: city building meshes, 2: landscaping,
//  3: fence/bays/branding/static props, 4: parked vehicle bodies.
// Camera/day/night-dependent city haze, lit windows and parked headlights are
// deliberately NOT stored here; drawSceneObjects() renders them every frame.
enum StaticListOffset {
    LIST_GROUND_ROADS = 0,
    LIST_CITY_BLOCKS,
    LIST_LANDSCAPE,
    LIST_INFRASTRUCTURE,
    LIST_PARKED_VEHICLES,
    STATIC_LIST_COUNT
};
GLuint staticListBase = 0;

// ============================================================================
// Utility helpers
// ============================================================================
float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}


float mixf(float a, float b, float t) {
    return a + (b - a) * clampf(t, 0.0f, 1.0f);
}


float goldenFactor() {
    return clampf(1.0f - std::fabs(dayNightBlend - 0.5f) * 2.0f, 0.0f, 1.0f);
}

void setMaterial(float r, float g, float b, float shininess = 32.0f,
                 float specularStrength = 0.65f, float alpha = 1.0f) {
   
    GLfloat ambient[]  = {0.32f*r, 0.32f*g, 0.32f*b, alpha};
    GLfloat diffuse[]  = {r, g, b, alpha};
    GLfloat specular[] = {specularStrength, specularStrength, specularStrength, alpha};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}


void setPBRMaterial(float r, float g, float b, float metallic = 0.35f, float roughness = 0.45f, float alpha = 1.0f) {
    metallic = clampf(metallic, 0.0f, 1.0f);
    roughness = clampf(roughness, 0.02f, 1.0f);
    float ao = mixf(0.17f, 0.24f, 1.0f - roughness);
    GLfloat ambient[] = {ao*r, ao*g, ao*b, alpha};
    GLfloat diffuse[] = {r*(1.0f - metallic*0.10f), g*(1.0f - metallic*0.10f), b*(1.0f - metallic*0.10f), alpha};
    float specBase = mixf(0.20f, 0.98f, metallic);
    float specStrength = specBase * mixf(1.08f, 0.34f, roughness);
    GLfloat specular[] = {specStrength, specStrength, specStrength, alpha};
    float shininess = mixf(8.0f, 118.0f, 1.0f - roughness);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void cube(float sx, float sy, float sz) {
    glPushMatrix();
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

void sphere(float r, int slices = 24, int stacks = 16) {
    glutSolidSphere(r, slices, stacks);
}

void cylinder(float radius, float height, int slices = 24) {
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);
    gluCylinder(q, radius, radius, height, slices, 4);
    gluDisk(q, 0.0, radius, slices, 1);
    glPushMatrix();
    glTranslatef(0, 0, height);
    gluDisk(q, 0.0, radius, slices, 1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

void cone(float baseRadius, float height, int slices = 24) {
    glutSolidCone(baseRadius, height, slices, 8);
}

void torus(float innerR, float outerR, int sides = 16, int rings = 28) {
    glutSolidTorus(innerR, outerR, sides, rings);
}

// ============================================================================
// Procedural texture creation (no external image files needed)
// ============================================================================
unsigned char hashNoise(int x, int y) {
    unsigned int n = (unsigned int)(x * 374761393u + y * 668265263u);
    n = (n ^ (n >> 13u)) * 1274126177u;
    return (unsigned char)((n ^ (n >> 16u)) & 0xFFu);
}

void uploadTexture(GLuint &id, const std::vector<unsigned char>& data, int w, int h) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
}

void makeRoadTexture(GLuint& id) {
    const int W = 128, H = 128;
    std::vector<unsigned char> d(W * H * 3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            unsigned char n1 = hashNoise(x * 3, y * 5);
            unsigned char n2 = hashNoise(x * 9 + 7, y * 11 + 17);
            unsigned char n3 = hashNoise(x * 15 + 19, y * 13 + 5);
            float fx = x / float(W - 1);
            float fy = y / float(H - 1);

         
            float wide = 2.5f * std::sin(fx * 8.5f) + 2.0f * std::cos(fy * 9.0f);
            int base = 235 + int(wide) + (n1 % 9) - 4;

            bool paleAggregate = (n2 % 7) == 0;
            bool darkAggregate = (n1 % 11) == 0;
            bool repairedPatch = (((x / 21) + (y / 19) + (n3 % 3)) % 6) == 0;
            bool microCrack = ((x + y * 2 + n3) % 57) < 2;
            bool softTireMark = ((x + n1) % 37) < 3 && y > 16 && y < H - 16;
            bool dustPatch = (((x - 26)*(x - 26) + (y - 94)*(y - 94)) < 145) || (((x - 96)*(x - 96) + (y - 34)*(y - 34)) < 180);

            int r = base + 4;
            int g = base + 3;
            int b = base;

            if (paleAggregate) { r += 7; g += 6; b += 4; }
            if (darkAggregate) { r -= 7; g -= 7; b -= 6; }
            if (repairedPatch) { r -= 8; g -= 8; b -= 7; }
            if (microCrack)    { r -= 13; g -= 13; b -= 12; }
            if (softTireMark)  { r -= 7; g -= 7; b -= 7; }
            if (dustPatch)     { r += 4; g += 3; b += 1; }

            // Lighter dusty edge similar to Dhaka urban roads.
            if (x < 8 || x > W - 9 || y < 8 || y > H - 9) {
                r += 4; g += 4; b += 2;
            }

            int i = (y * W + x) * 3;
            d[i+0] = (unsigned char)clampf((float)r, 0, 255);
            d[i+1] = (unsigned char)clampf((float)g, 0, 255);
            d[i+2] = (unsigned char)clampf((float)b, 0, 255);
        }
    }
    uploadTexture(id, d, W, H);
}

void makeBrickTexture(GLuint& id) {
    const int W = 128, H = 128;
    std::vector<unsigned char> d(W * H * 3);
    const int brickW = 32, brickH = 16, mortar = 2;
    for (int y = 0; y < H; ++y) {
        int row = y / brickH;
        int offset = (row % 2) ? brickW / 2 : 0;
        for (int x = 0; x < W; ++x) {
            int localX = (x + offset) % brickW;
            int localY = y % brickH;
            int i = (y * W + x) * 3;
            bool isMortar = localX < mortar || localY < mortar;
            if (isMortar) {
                d[i+0] = 190; d[i+1] = 182; d[i+2] = 170;
            } else {
                unsigned char n = hashNoise(x, y);
                d[i+0] = (unsigned char)(145 + n % 32);
                d[i+1] = (unsigned char)(55 + n % 20);
                d[i+2] = (unsigned char)(36 + n % 16);
            }
        }
    }
    uploadTexture(id, d, W, H);
}

void makeWoodTexture(GLuint& id) {
    const int W = 128, H = 128;
    std::vector<unsigned char> d(W * H * 3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float grain = std::sin((x + 0.17f*y) * 0.28f) * 14.0f;
            unsigned char n = hashNoise(x, y);
            int jitter = (n % 13) - 6;
            int i = (y * W + x) * 3;
            d[i+0] = (unsigned char)clampf(120 + grain + jitter, 0, 255);
            d[i+1] = (unsigned char)clampf(70 + grain*0.45f + jitter, 0, 255);
            d[i+2] = (unsigned char)clampf(32 + grain*0.20f + jitter/2, 0, 255);
        }
    }
    uploadTexture(id, d, W, H);
}

void makeMetalTexture(GLuint& id) {
    const int W = 128, H = 128;
    std::vector<unsigned char> d(W * H * 3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int stripe = ((x / 8) % 2) ? 18 : 0;
            unsigned char n = hashNoise(x, y);
            int base = 92 + stripe + (n % 10);
            int i = (y * W + x) * 3;
            d[i+0] = (unsigned char)clampf(base, 0, 255);
            d[i+1] = (unsigned char)clampf(base + 7, 0, 255);
            d[i+2] = (unsigned char)clampf(base + 17, 0, 255);
        }
    }
    uploadTexture(id, d, W, H);
}

void makePavementTexture(GLuint& id) {
    const int W = 128, H = 128;
    std::vector<unsigned char> d(W * H * 3);
    const int tile = 16;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            bool line = (x % tile < 2) || (y % tile < 2);
            unsigned char n = hashNoise(x, y);
            int i = (y * W + x) * 3;
            if (line) {
                d[i+0] = 105; d[i+1] = 108; d[i+2] = 112;
            } else {
                int base = 150 + (n % 22) - 11;
                d[i+0] = (unsigned char)base;
                d[i+1] = (unsigned char)(base + 2);
                d[i+2] = (unsigned char)(base + 4);
            }
        }
    }
    uploadTexture(id, d, W, H);
}

void makeGrassTexture(GLuint& id) {
    const int W = 128, H = 128;
    std::vector<unsigned char> d(W * H * 3);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            unsigned char n1 = hashNoise(x * 2, y * 3);
            unsigned char n2 = hashNoise(x * 7 + 5, y * 5 + 11);
            unsigned char n3 = hashNoise(x * 11 + 23, y * 13 + 3);
            float fx = x / float(W - 1);
            float fy = y / float(H - 1);
            float band = 10.0f * std::sin(fx * 9.0f) + 7.0f * std::cos(fy * 11.0f);
            bool deepPatch = (((x / 15) + (y / 11)) % 5) == 0;
            bool sunTip  = ((n2 + x + y) % 17) == 0;
            bool shadePatch = ((n3 + x*2 - y) % 23) == 0;

            // Natural tropical lawn: deep olive greens instead of neon green.
            int r = 42 + (n1 % 12);
            int g = 104 + int(band * 0.65f) + (n1 % 22) - 10;
            int b = 31 + (n2 % 10);

            if (deepPatch) { r -= 6; g -= 16; b -= 4; }
            if (sunTip)    { r += 11; g += 9;  b -= 1; }
            if (shadePatch){ r -= 2; g += 4;  b += 1; }

            int i = (y * W + x) * 3;
            d[i+0] = (unsigned char)clampf((float)r, 0, 255);
            d[i+1] = (unsigned char)clampf((float)g, 0, 255);
            d[i+2] = (unsigned char)clampf((float)b, 0, 255);
        }
    }
    uploadTexture(id, d, W, H);
}

void initTextures() {
    makeRoadTexture(texRoad);
    makeBrickTexture(texBrick);
    makeWoodTexture(texWood);
    makeMetalTexture(texMetal);
    makePavementTexture(texPavement);
    makeGrassTexture(texGrass);
}

void beginTexture(GLuint tex) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void endTexture() {
    glDisable(GL_TEXTURE_2D);
}

void texturedQuadXZ(float x1, float z1, float x2, float z2, float y,
                    GLuint tex, float repX = 4.0f, float repZ = 4.0f) {
    const bool roadSurface = (tex == texRoad);

    glDisable(GL_CULL_FACE);

   
    if (roadSurface) {
        beginTexture(tex);
        glDisable(GL_LIGHTING);
        glColor3f(mixf(0.20f, 0.04f, dayNightBlend),
                  mixf(0.20f, 0.04f, dayNightBlend),
                  mixf(0.21f, 0.05f, dayNightBlend)); // V65: day charcoal <-> night black
    } else {
        beginTexture(tex);
        setMaterial(1, 1, 1, 18, 0.20f);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // V64 CRITICAL FIX 1: vertices wound strictly Counter-Clockwise as seen
    // from above (+Y, the direction glNormal3f points): (x1,z1) -> (x1,z2) ->
    // (x2,z2) -> (x2,z1). The previous vertex order (x1,z1)->(x2,z1)->(x2,z2)
    // ->(x1,z2) was wound Clockwise from above, so with GL_CULL_FACE/GL_BACK
    // enabled the quad's front face pointed straight DOWN and every top-down
    // camera saw only the sky-blue glClearColor "through" the missing road -
    // this was the exact light-blue/grey tint reported.
    glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);       glVertex3f(x1, y, z1);
        glTexCoord2f(0, repZ);    glVertex3f(x1, y, z2);
        glTexCoord2f(repX, repZ); glVertex3f(x2, y, z2);
        glTexCoord2f(repX, 0);    glVertex3f(x2, y, z1);
    glEnd();

    endTexture();
    if (roadSurface) {
        glColor4f(1.0f,1.0f,1.0f,1.0f);
        glEnable(GL_LIGHTING);
    }

    glEnable(GL_CULL_FACE);
}


void darkGroundQuadXZ(float x1, float z1, float x2, float z2, float y,
                      float repX = 4.0f, float repZ = 4.0f,
                      float shade = 0.075f) {
    (void)repX;
    (void)repZ;

    // V64 CRITICAL FIX 1: same back-face-culling guard as texturedQuadXZ, so
    // outdoor ground/plaza patches can never disappear from a top-down view.
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    const float dayShade = clampf(shade + 0.155f, 0.0f, 0.30f);
    const float finalShade = mixf(dayShade, shade, dayNightBlend);
    glColor3f(finalShade, finalShade, finalShade);

    // V64 CRITICAL FIX 1: corrected Counter-Clockwise winding (seen from +Y) -
    // see the matching comment in texturedQuadXZ for the full explanation.
    glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0);       glVertex3f(x1, y, z1);
        glTexCoord2f(0, repZ);    glVertex3f(x1, y, z2);
        glTexCoord2f(repX, repZ); glVertex3f(x2, y, z2);
        glTexCoord2f(repX, 0);    glVertex3f(x2, y, z1);
    glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
}

void texturedBox(float sx, float sy, float sz, GLuint tex,
                 float repX = 2.0f, float repY = 2.0f) {
    float x = sx * 0.5f, y = sy * 0.5f, z = sz * 0.5f;
    beginTexture(tex);
    setMaterial(1, 1, 1, 30, 0.45f);
    glBegin(GL_QUADS);
        // Front (+Z)
        glNormal3f(0,0,1);
        glTexCoord2f(0,0); glVertex3f(-x,-y, z);
        glTexCoord2f(repX,0); glVertex3f( x,-y, z);
        glTexCoord2f(repX,repY); glVertex3f( x, y, z);
        glTexCoord2f(0,repY); glVertex3f(-x, y, z);
        // Back (-Z)
        glNormal3f(0,0,-1);
        glTexCoord2f(0,0); glVertex3f( x,-y,-z);
        glTexCoord2f(repX,0); glVertex3f(-x,-y,-z);
        glTexCoord2f(repX,repY); glVertex3f(-x, y,-z);
        glTexCoord2f(0,repY); glVertex3f( x, y,-z);
        // Right (+X)
        glNormal3f(1,0,0);
        glTexCoord2f(0,0); glVertex3f( x,-y, z);
        glTexCoord2f(repX,0); glVertex3f( x,-y,-z);
        glTexCoord2f(repX,repY); glVertex3f( x, y,-z);
        glTexCoord2f(0,repY); glVertex3f( x, y, z);
        // Left (-X)
        glNormal3f(-1,0,0);
        glTexCoord2f(0,0); glVertex3f(-x,-y,-z);
        glTexCoord2f(repX,0); glVertex3f(-x,-y, z);
        glTexCoord2f(repX,repY); glVertex3f(-x, y, z);
        glTexCoord2f(0,repY); glVertex3f(-x, y,-z);
        // Top (+Y)
        glNormal3f(0,1,0);
        glTexCoord2f(0,0); glVertex3f(-x, y, z);
        glTexCoord2f(repX,0); glVertex3f( x, y, z);
        glTexCoord2f(repX,repY); glVertex3f( x, y,-z);
        glTexCoord2f(0,repY); glVertex3f(-x, y,-z);
        // Bottom (-Y)
        glNormal3f(0,-1,0);
        glTexCoord2f(0,0); glVertex3f(-x,-y,-z);
        glTexCoord2f(repX,0); glVertex3f( x,-y,-z);
        glTexCoord2f(repX,repY); glVertex3f( x,-y, z);
        glTexCoord2f(0,repY); glVertex3f(-x,-y, z);
    glEnd();
    endTexture();
}

// ============================================================================
// Text / HUD helpers
// ============================================================================
void drawBitmapText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2f(x, y);
    for (size_t i = 0; i < s.size(); ++i) glutBitmapCharacter(font, s[i]);
}

void drawHUD() {
    if (!showHUD) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawBitmapText(14, winH - 22, "SMART 3D BUS TERMINAL & ECO TRANSPORT HUB", GLUT_BITMAP_HELVETICA_18);
    drawBitmapText(14, winH - 42, "Camera: W/S/A/D, Q/E | Look: arrows / left mouse | 1-7 FIXED MANUAL CINEMATIC SHOTS");
    drawBitmapText(14, winH - 58, "Object: I/K Z, J/L X, U/O rotate, +/- scale | Right drag object | Ctrl+wheel scale");
    drawBitmapText(14, winH - 74, "R rotation | M traffic | N day-night | P rain | C tour | H HUD | X axes");

    std::ostringstream oss;
    oss << "Mode: " << (nightMode ? "Night" : "Day")
        << "   Camera shot: " << cameraPreset
        << "   Cinematic: " << (cinematicTour ? "ON" : "OFF")
        << "   Day-Night blend: " << dayNightBlend
        << "   Object scale: " << objectScale
        << "   FPS: " << fpsValue;
    drawBitmapText(14, 16, oss.str());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawAxes3D(float s = 3.0f) {
    if (!showAxes) return;
    glDisable(GL_LIGHTING);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
        glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(s,0,0);
        glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,s,0);
        glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,s);
    glEnd();
    glLineWidth(1.0f);
    glColor3f(1,1,1);
    glEnable(GL_LIGHTING);
}



void drawSoftShadow(float x, float z, float rx, float rz, float alpha = 0.18f) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Fixed footprint: static objects may live in a display list, so shadow
    // geometry must not depend on the day/night value present at compile time.
    const float shadowStretch = 1.0f;
    const float lightDirX = -PRIMARY_LIGHT_X;
    const float lightDirZ = -PRIMARY_LIGHT_Z;

    for (int layer = 0; layer < 3; ++layer) {
        float expand = 1.0f + layer * 0.16f;
        float layerAlpha = alpha * (layer == 0 ? 1.00f : (layer == 1 ? 0.55f : 0.28f));
        glColor4f(0.0f, 0.0f, 0.0f, layerAlpha);
        glPushMatrix();
        glTranslatef(x + lightDirX * layer * 0.22f, 0.033f + layer * 0.002f, z + lightDirZ * layer * 0.18f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0, 0, 0);
        for (int i = 0; i <= 44; ++i) {
            float a = 2.0f * PI * i / 44.0f;
            float ex = std::cos(a) * rx * expand * shadowStretch;
            float ez = std::sin(a) * rz * expand;
            glVertex3f(ex, 0.0f, ez);
        }
        glEnd();
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawLightPool(float x, float z, float radius, float r, float g, float b, float alpha = 0.15f) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glPushMatrix();
    glTranslatef(x, 0.04f, z);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, alpha);
    glVertex3f(0, 0, 0);
    glColor4f(r, g, b, 0.0f);
    for (int i = 0; i <= 40; ++i) {
        float a = 2.0f * PI * i / 40.0f;
        glVertex3f(std::cos(a) * radius, 0.0f, std::sin(a) * radius);
    }
    glEnd();
    glPopMatrix();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ============================================================================
void drawDashedLineZ(float x, float z1, float z2, float dash = 2.2f, float gap = 1.6f,
                     float width = 0.10f, bool yellow = false) {
    glDisable(GL_LIGHTING);
    glColor3f(yellow ? 1.0f : 0.95f, yellow ? 0.82f : 0.95f, yellow ? 0.05f : 0.95f);
    for (float z = z1; z < z2; z += dash + gap) {
        float len = std::min(dash, z2 - z);
        glPushMatrix();
        glTranslatef(x, 0.045f, z + len * 0.5f);
        cube(width, 0.03f, len);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawDashedLineX(float z, float x1, float x2, float dash = 2.2f, float gap = 1.6f,
                     float width = 0.10f, bool yellow = false) {
    glDisable(GL_LIGHTING);
    glColor3f(yellow ? 1.0f : 0.95f, yellow ? 0.82f : 0.95f, yellow ? 0.05f : 0.95f);
    for (float x = x1; x < x2; x += dash + gap) {
        float len = std::min(dash, x2 - x);
        glPushMatrix();
        glTranslatef(x + len * 0.5f, 0.045f, z);
        cube(len, 0.03f, width);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawCrosswalk(float x, float z, bool acrossX) {
    // V64 CRITICAL FIX 2: high-visibility safety-yellow zebra crossing
    // (RGB 1.0, 0.82, 0.05) instead of the previous near-white stripes.
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.82f, 0.05f);
    for (int i = -4; i <= 4; ++i) {
        glPushMatrix();
        if (acrossX) glTranslatef(x + i * 0.58f, 0.052f, z);
        else         glTranslatef(x, 0.052f, z + i * 0.58f);
        if (acrossX) cube(0.34f, 0.03f, 4.35f);
        else         cube(4.35f, 0.03f, 0.34f);
        glPopMatrix();
    }

    // Slightly worn, darker yellow outer border for more realistic thickness.
    glColor3f(0.80f, 0.64f, 0.02f);
    if (acrossX) {
        glPushMatrix(); glTranslatef(x, 0.049f, z - 2.30f); cube(5.5f, 0.01f, 0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef(x, 0.049f, z + 2.30f); cube(5.5f, 0.01f, 0.08f); glPopMatrix();
    } else {
        glPushMatrix(); glTranslatef(x - 2.30f, 0.049f, z); cube(0.08f, 0.01f, 5.5f); glPopMatrix();
        glPushMatrix(); glTranslatef(x + 2.30f, 0.049f, z); cube(0.08f, 0.01f, 5.5f); glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

// V64 CRITICAL FIX 2: realistic "Yellow Box Junction" hazard marking - the
// diagonal criss-cross paint used at real signal-controlled intersections to
// warn drivers not to enter the junction unless their exit lane is clear.
// Two families of parallel diagonal stripes (+45 and -45 degrees) cross the
// square box, framed by a solid yellow border so it reads clearly against
// the plain black asphalt.
void drawBoxJunctionCross(float cx, float cz, float size) {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.82f, 0.05f);

    const float half = size * 0.5f;
    const float stripeLen = size * 1.05f;
    const float stripeW = 0.11f;
    const int stripeCount = 5;
    const float spacing = size / (stripeCount + 1);

    glPushMatrix();
    glTranslatef(cx, 0.053f, cz);

    // Diagonal stripes leaning one way ( / ).
    for (int i = 1; i <= stripeCount; ++i) {
        float offset = -half + i * spacing;
        glPushMatrix();
        glTranslatef(offset, 0.0f, 0.0f);
        glRotatef(45.0f, 0, 1, 0);
        cube(stripeW, 0.02f, stripeLen);
        glPopMatrix();
    }

    // Diagonal stripes leaning the other way ( \ ).
    for (int i = 1; i <= stripeCount; ++i) {
        float offset = -half + i * spacing;
        glPushMatrix();
        glTranslatef(offset, 0.0f, 0.0f);
        glRotatef(-45.0f, 0, 1, 0);
        cube(stripeW, 0.02f, stripeLen);
        glPopMatrix();
    }

    // Solid yellow border outline so the box reads clearly as one unit.
    glPushMatrix(); glTranslatef(0, 0.001f, -half); cube(size, 0.02f, 0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0.001f,  half); cube(size, 0.02f, 0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(-half, 0.001f, 0); cube(0.12f, 0.02f, size); glPopMatrix();
    glPushMatrix(); glTranslatef( half, 0.001f, 0); cube(0.12f, 0.02f, size); glPopMatrix();

    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawCurbs() {
    glDisable(GL_LIGHTING);
    // V65: bright concrete curb by day, blending to the original dim tone at
    // night so the road/curb/building contrast reads clearly in sunlight.
    const float curbTone = mixf(0.58f, 0.34f, dayNightBlend);
    glColor3f(curbTone, curbTone, curbTone);
    // Around central terminal island
    glPushMatrix(); glTranslatef(-12.0f, 0.14f, 0); cube(0.30f, 0.28f, 24.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 12.0f, 0.14f, 0); cube(0.30f, 0.28f, 24.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0.14f, -12.0f); cube(24.0f, 0.28f, 0.30f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0.14f,  12.0f); cube(24.0f, 0.28f, 0.30f); glPopMatrix();
    glEnable(GL_LIGHTING);
}


// ============================================================================
// Realistic road details - V8 Road Upgrade
// ============================================================================
void drawOilStain(float x, float z, float sx, float sz, float alpha);
void drawPothole(float x, float z, float sx, float sz);
void drawRoadDustPatch(float x, float z, float sx, float sz, float alpha);
void drawRoadEdgeLine(float x, float z, float sx, float sz);
void drawBoxJunctionCross(float cx, float cz, float size);
void drawRoadShoulder(float x, float z, float sx, float sz) {
    glDisable(GL_LIGHTING);
    glColor3f(0.075f,0.075f,0.075f);
    glPushMatrix();
    glTranslatef(x,0.032f,z);
    cube(sx,0.064f,sz);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // dusty transition strip near shoulder edge
    drawRoadDustPatch(x, z, sx * 2.4f, sz * 0.98f, 0.09f);
}

void drawSidewalk(float x, float z, float sx, float sz) {
    // The old code forgot to translate the sidewalk body to (x,z), creating
    // four overlapping slabs at the world origin. Draw both slab and cap with
    // explicit, unlit neutral colours so they cannot inherit sky lighting.
    // V65: both tones now blend to bright daytime concrete grey, keeping the
    // original dim tone at night (dayNightBlend = 1.0) unchanged.
    glDisable(GL_LIGHTING);
    const float slabTone = mixf(0.56f, 0.105f, dayNightBlend);
    const float capTone  = mixf(0.62f, 0.18f,  dayNightBlend);
    glColor3f(slabTone, slabTone, slabTone);
    glPushMatrix(); glTranslatef(x,0.060f,z); cube(sx,0.12f,sz); glPopMatrix();
    glColor3f(capTone, capTone, capTone);
    glPushMatrix();
    glTranslatef(x, 0.135f, z);
    if (sx < sz) cube(sx * 0.92f, 0.03f, sz * 0.995f);
    else         cube(sx * 0.995f, 0.03f, sz * 0.92f);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawRoadCracks(float x, float z, bool vertical=false) {
    glDisable(GL_LIGHTING);
    glColor3f(0.20f,0.20f,0.20f);
    glLineWidth(1.7f);
    glBegin(GL_LINES);
    if(vertical) {
        glVertex3f(x,0.020f, z);
        glVertex3f(x+0.7f,0.020f,z+1.8f);
        glVertex3f(x+0.7f,0.020f,z+1.8f);
        glVertex3f(x-0.2f,0.020f,z+3.4f);
        glVertex3f(x+0.15f,0.020f,z+0.8f);
        glVertex3f(x+1.1f,0.020f,z+1.1f);
    } else {
        glVertex3f(x,0.020f,z);
        glVertex3f(x+1.8f,0.020f,z+0.3f);
        glVertex3f(x+1.8f,0.020f,z+0.3f);
        glVertex3f(x+3.1f,0.020f,z-0.25f);
        glVertex3f(x+0.9f,0.020f,z+0.15f);
        glVertex3f(x+1.3f,0.020f,z+1.0f);
    }
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

void drawBusStopMarking(float x,float z) {
    // V64: matches the same safety-yellow used for crosswalks/box junctions.
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.82f,0.05f);
    glPushMatrix();
    glTranslatef(x,0.055f,z);
    cube(6.2f,0.025f,0.12f);
    glPopMatrix();

    // Bus bay stripes
    for(float i=-2.2f;i<=2.2f;i+=0.55f){
        glPushMatrix();
        glTranslatef(x+i,0.058f,z+0.35f);
        cube(0.12f,0.025f,0.85f);
        glPopMatrix();
    }

    // simplified BUS text marking from block segments
    glColor3f(0.94f,0.92f,0.78f);
    float sx = x - 1.2f;
    glPushMatrix(); glTranslatef(sx,0.058f,z-0.55f); cube(0.12f,0.02f,0.75f); glPopMatrix();
    glPushMatrix(); glTranslatef(sx+0.24f,0.058f,z-0.55f); cube(0.12f,0.02f,0.75f); glPopMatrix();
    glPushMatrix(); glTranslatef(sx+0.12f,0.058f,z-0.87f); cube(0.18f,0.02f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(sx+0.12f,0.058f,z-0.55f); cube(0.18f,0.02f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(sx+0.12f,0.058f,z-0.23f); cube(0.18f,0.02f,0.10f); glPopMatrix();
    glEnable(GL_LIGHTING);

    drawRoadDustPatch(x, z + 0.85f, 6.6f, 0.8f, 0.10f);
}

void drawParkingLane(float x,float z,float w,float d){
    glDisable(GL_LIGHTING);
    glColor3f(0.95f,0.95f,0.95f);
    glBegin(GL_LINE_LOOP);
        glVertex3f(x-w/2,0.06f,z-d/2);
        glVertex3f(x+w/2,0.06f,z-d/2);
        glVertex3f(x+w/2,0.06f,z+d/2);
        glVertex3f(x-w/2,0.06f,z+d/2);
    glEnd();
    glEnable(GL_LIGHTING);
}


void drawOilStain(float x, float z, float sx, float sz, float alpha = 0.24f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glColor4f(0.025f, 0.025f, 0.025f, alpha);
    glPushMatrix(); glTranslatef(x, 0.041f, z); cube(sx, 0.008f, sz); glPopMatrix();
    glColor4f(0.060f, 0.060f, 0.060f, alpha * 0.45f);
    glPushMatrix(); glTranslatef(x + 0.08f, 0.042f, z + 0.06f); cube(sx * 0.55f, 0.007f, sz * 0.50f); glPopMatrix();
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawPothole(float x, float z, float sx, float sz) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glColor4f(0.025f, 0.025f, 0.025f, 0.65f);
    glPushMatrix(); glTranslatef(x, 0.028f, z); cube(sx, 0.015f, sz); glPopMatrix();
    glColor4f(0.075f, 0.075f, 0.075f, 0.35f);
    glPushMatrix(); glTranslatef(x, 0.038f, z); cube(sx * 1.18f, 0.004f, sz * 1.18f); glPopMatrix();
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawRoadDustPatch(float x, float z, float sx, float sz, float alpha = 0.18f) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glColor4f(0.54f, 0.50f, 0.43f, alpha);
    glPushMatrix(); glTranslatef(x, 0.042f, z); cube(sx, 0.006f, sz); glPopMatrix();
    glEnable(GL_LIGHTING);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawRoadEdgeLine(float x, float z, float sx, float sz) {
    glDisable(GL_LIGHTING);
    glColor3f(0.88f, 0.88f, 0.85f);
    glPushMatrix(); glTranslatef(x, 0.052f, z); cube(sx, 0.018f, sz); glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawTrafficRoadSign(float x,float z){
    glPushMatrix();
    glTranslatef(x,0,z);
    setMaterial(0.08f,0.09f,0.10f,80,0.9f);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    cylinder(0.04f,3.0f,12);
    glPopMatrix();
    setMaterial(0.95f,0.75f,0.05f,60,0.7f);
    glPushMatrix();
    glTranslatef(0,3.2f,0);
    cube(0.9f,0.65f,0.08f);
    glPopMatrix();
    glPopMatrix();
}

void drawAsphaltStrip(float x, float z, float sx, float sz, float y, float r, float g, float b, float alpha = 1.0f) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    // Convert any caller colour to one neutral intensity. Even a future caller
    // passing unequal RGB values therefore cannot introduce a blue road patch.
    const float neutral = ((r+g+b)/3.0f) * 0.16f;
    glColor4f(neutral,neutral,neutral,alpha);
    glPushMatrix();
    glTranslatef(x, y, z);
    cube(sx, 0.01f, sz);
    glPopMatrix();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}


void drawSkyCloudCluster(float x, float y, float z, float scale, float alpha) {
    glPushMatrix();
    glTranslatef(x, y, z);

    // V68: matched to a flat-illustration reference sky - clouds read as
    // clean, bright, simple cumulus shapes rather than a muted photographic
    // grey. Underside is a soft light-grey shadow tone, not dark storm-grey.
    glColor4f(mixf(0.87f,0.16f,dayNightBlend),
              mixf(0.89f,0.18f,dayNightBlend),
              mixf(0.92f,0.24f,dayNightBlend), alpha * 0.82f);
    glPushMatrix(); glScalef(scale*2.45f,scale*0.34f,scale*0.82f); sphere(1.0f,18,12); glPopMatrix();

    // Bright, near-pure-white cloud tops for a clean flat-design look.
    glColor4f(mixf(0.99f,0.34f,dayNightBlend),
              mixf(0.99f,0.36f,dayNightBlend),
              1.00f,
              alpha);
    const float lobes[][4] = {
        {-1.28f,0.18f,0.04f,0.82f}, {-0.55f,0.42f,0.00f,1.08f},
        { 0.28f,0.58f,0.06f,1.22f}, { 1.15f,0.30f,0.02f,0.92f},
        { 1.78f,0.12f,0.00f,0.62f}
    };
    for (int i=0;i<5;++i) {
        glPushMatrix();
        glTranslatef(lobes[i][0]*scale,lobes[i][1]*scale,lobes[i][2]*scale);
        glScalef(lobes[i][3]*scale,lobes[i][3]*0.72f*scale,lobes[i][3]*0.82f*scale);
        sphere(1.0f,18,12);
        glPopMatrix();
    }
    glPopMatrix();
}

// V67: draws one large flat vertical sky panel as 4 stacked gradient bands
// (instead of 2) for a visibly smoother top-to-horizon transition, reducing
// the hard colour seams a 2-stop gradient can show. (x1,z1)-(x2,z2) give the
// panel's bottom corners; pass the same value twice for whichever axis is
// meant to stay fixed (e.g. z1==z2 for the front/back walls, x1==x2 for the
// left/right walls). GL_CULL_FACE is already disabled by the caller, so
// vertex winding here doesn't need to match any particular direction.
void drawSkyGradientPanel(float x1, float z1, float x2, float z2,
                          float topR,float topG,float topB,
                          float upperR,float upperG,float upperB,
                          float midR,float midG,float midB,
                          float lowerR,float lowerG,float lowerB,
                          float hazeR,float hazeG,float hazeB) {
    const float yTop=62.0f, yUpper=41.0f, yMid=20.0f, yLower=8.5f, yBot=-3.0f;
    auto band = [&](float yA,float yB,float rA,float gA,float bA,float rB,float gB,float bB){
        glBegin(GL_QUADS);
            glColor3f(rA,gA,bA); glVertex3f(x1,yA,z1); glVertex3f(x2,yA,z2);
            glColor3f(rB,gB,bB); glVertex3f(x2,yB,z2); glVertex3f(x1,yB,z1);
        glEnd();
    };
    band(yTop,yUpper,   topR,topG,topB,       upperR,upperG,upperB);
    band(yUpper,yMid,   upperR,upperG,upperB, midR,midG,midB);
    band(yMid,yLower,   midR,midG,midB,       lowerR,lowerG,lowerB);
    band(yLower,yBot,   lowerR,lowerG,lowerB, hazeR,hazeG,hazeB);
}

void drawSkyBackdrop() {
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    // V68: matched to a flat-illustration reference sky - bright cyan-blue at
    // the zenith fading through a soft mint-teal into a pale warm
    // yellow-green right at the horizon (the "flat city illustration" look,
    // rather than the more photographic blue-to-white-haze gradient tried in
    // V66). Night values (the second argument to each mixf) are unchanged.
    float skyTopR = mixf(0.28f, 0.03f, dayNightBlend);
    float skyTopG = mixf(0.72f, 0.05f, dayNightBlend);
    float skyTopB = mixf(0.92f, 0.12f, dayNightBlend);

    float skyMidR = mixf(0.55f, 0.05f, dayNightBlend);
    float skyMidG = mixf(0.82f, 0.07f, dayNightBlend);
    float skyMidB = mixf(0.80f, 0.18f, dayNightBlend);

    float skyHazeR = mixf(0.85f, 0.07f, dayNightBlend);
    float skyHazeG = mixf(0.90f, 0.09f, dayNightBlend);
    float skyHazeB = mixf(0.62f, 0.16f, dayNightBlend);

    // V67: 3-STAGE DAY CYCLE - a warm golden-hour tint fades in and back out
    // automatically right at the day<->night transition midpoint (see
    // goldenFactor()), so the sky briefly reads as a sunset/sunrise instead
    // of jumping straight from blue-day to black-night. At golden=0 (pure
    // day or pure night) this changes nothing.
    const float golden = goldenFactor();
    skyTopR  = mixf(skyTopR,  0.42f, golden*0.55f);
    skyTopG  = mixf(skyTopG,  0.22f, golden*0.55f);
    skyTopB  = mixf(skyTopB,  0.38f, golden*0.55f);
    skyMidR  = mixf(skyMidR,  0.86f, golden*0.60f);
    skyMidG  = mixf(skyMidG,  0.42f, golden*0.60f);
    skyMidB  = mixf(skyMidB,  0.38f, golden*0.60f);
    skyHazeR = mixf(skyHazeR, 1.00f, golden*0.65f);
    skyHazeG = mixf(skyHazeG, 0.62f, golden*0.65f);
    skyHazeB = mixf(skyHazeB, 0.38f, golden*0.65f);

    // V67: two extra intermediate colour stops turn the previous 2-band
    // gradient into a smoother 4-band one (used by drawSkyGradientPanel).
    const float skyUpperR = mixf(skyTopR, skyMidR, 0.5f);
    const float skyUpperG = mixf(skyTopG, skyMidG, 0.5f);
    const float skyUpperB = mixf(skyTopB, skyMidB, 0.5f);
    const float skyLowerR = mixf(skyMidR, skyHazeR, 0.5f);
    const float skyLowerG = mixf(skyMidG, skyHazeG, 0.5f);
    const float skyLowerB = mixf(skyMidB, skyHazeB, 0.5f);

    // Back, front, left and right vertical walls of the sky box.
    drawSkyGradientPanel(-120.0f,-118.0f, 120.0f,-118.0f,
                         skyTopR,skyTopG,skyTopB, skyUpperR,skyUpperG,skyUpperB,
                         skyMidR,skyMidG,skyMidB, skyLowerR,skyLowerG,skyLowerB,
                         skyHazeR,skyHazeG,skyHazeB);
    drawSkyGradientPanel(-120.0f, 118.0f, 120.0f, 118.0f,
                         skyTopR,skyTopG,skyTopB, skyUpperR,skyUpperG,skyUpperB,
                         skyMidR,skyMidG,skyMidB, skyLowerR,skyLowerG,skyLowerB,
                         skyHazeR,skyHazeG,skyHazeB);
    drawSkyGradientPanel(-118.0f,-118.0f, -118.0f, 118.0f,
                         skyTopR,skyTopG,skyTopB, skyUpperR,skyUpperG,skyUpperB,
                         skyMidR,skyMidG,skyMidB, skyLowerR,skyLowerG,skyLowerB,
                         skyHazeR,skyHazeG,skyHazeB);
    drawSkyGradientPanel( 118.0f,-118.0f,  118.0f, 118.0f,
                         skyTopR,skyTopG,skyTopB, skyUpperR,skyUpperG,skyUpperB,
                         skyMidR,skyMidG,skyMidB, skyLowerR,skyLowerG,skyLowerB,
                         skyHazeR,skyHazeG,skyHazeB);

    // Sky ceiling prevents aerial camera views from falling back to a
    // flat clear colour instead of the same photographic sky palette.
    glBegin(GL_QUADS);
        glColor3f(skyTopR, skyTopG, skyTopB);
        glVertex3f(-120.0f, 62.0f,-118.0f);
        glVertex3f(-120.0f, 62.0f, 118.0f);
        glVertex3f( 120.0f, 62.0f, 118.0f);
        glVertex3f( 120.0f, 62.0f,-118.0f);
    glEnd();

    // Horizon haze band for depth.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
        glColor4f(skyHazeR, skyHazeG, skyHazeB, mixf(0.32f, 0.12f, dayNightBlend));
        glVertex3f(-118.0f, 10.0f, -116.0f);
        glVertex3f( 118.0f, 10.0f, -116.0f);
        glColor4f(skyHazeR, skyHazeG, skyHazeB, 0.0f);
        glVertex3f( 118.0f, 23.0f, -116.0f);
        glVertex3f(-118.0f, 23.0f, -116.0f);
    glEnd();

   
    float skylineR = mixf(0.62f,0.09f,dayNightBlend);
    float skylineG = mixf(0.68f,0.11f,dayNightBlend);
    float skylineB = mixf(0.66f,0.16f,dayNightBlend);
    skylineR = mixf(skylineR, 0.50f, golden*0.4f);
    skylineG = mixf(skylineG, 0.30f, golden*0.4f);
    skylineB = mixf(skylineB, 0.34f, golden*0.4f);
    glColor4f(skylineR, skylineG, skylineB, 1.0f);
    for (int i = -14; i <= 14; ++i) {
        float x = i * 7.8f;
        float h = 6.5f + std::fabs((i % 6) - 2) * 1.45f + ((i % 5) == 0 ? 7.0f : 0.0f);
        float halfW = 2.7f + 0.35f * (std::abs(i) % 3);
        glBegin(GL_QUADS);
            glVertex3f(x-halfW,-0.3f,-94.0f); glVertex3f(x+halfW,-0.3f,-94.0f);
            glVertex3f(x+halfW,h,-94.0f);     glVertex3f(x-halfW,h,-94.0f);
        glEnd();
    }

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

void drawExtraRoadDetail() {
    // Darker wheel-wear strips, patch repairs and edges for more realistic asphalt.
    drawAsphaltStrip(-17.0f, 0.0f, 0.80f, 39.2f, 0.018f, 0.22f, 0.22f, 0.22f, 0.34f);
    drawAsphaltStrip( 17.0f, 0.0f, 0.80f, 39.2f, 0.018f, 0.22f, 0.22f, 0.22f, 0.34f);
    drawAsphaltStrip(  0.0f,-16.0f, 42.0f, 0.80f, 0.018f, 0.22f, 0.22f, 0.22f, 0.34f);
    drawAsphaltStrip(  0.0f, 16.0f, 42.0f, 0.80f, 0.018f, 0.22f, 0.22f, 0.22f, 0.34f);

    // Outer roads
    drawAsphaltStrip(-27.0f, 0.0f, 0.60f, 45.0f, 0.018f, 0.22f, 0.22f, 0.22f, 0.26f);
    drawAsphaltStrip( 27.0f, 0.0f, 0.60f, 45.0f, 0.018f, 0.22f, 0.22f, 0.22f, 0.26f);
    drawAsphaltStrip(  0.0f, 23.0f, 48.0f, 0.60f, 0.018f, 0.22f, 0.22f, 0.22f, 0.26f);

    // Patch repairs
    drawAsphaltStrip(-6.8f, -18.0f, 6.4f, 1.15f, 0.019f, 0.28f, 0.28f, 0.28f, 0.38f);
    drawAsphaltStrip(10.5f,  12.8f, 5.8f, 0.95f, 0.019f, 0.28f, 0.28f, 0.28f, 0.38f);
    drawAsphaltStrip(28.8f,   5.0f, 1.15f, 6.7f, 0.019f, 0.28f, 0.28f, 0.28f, 0.38f);

    // Oil stains near stopping / turning areas.
    drawOilStain( 0.0f, -18.2f, 1.8f, 1.2f, 0.26f);
    drawOilStain(-16.8f, 1.5f, 1.3f, 0.9f, 0.22f);
    drawOilStain( 16.8f, -3.5f, 1.6f, 1.0f, 0.22f);
    drawOilStain(33.4f, 17.6f, 1.1f, 0.8f, 0.18f);

    // Small potholes / worn depressions.
    drawPothole(-15.6f, 9.8f, 0.70f, 0.46f);
    drawPothole(18.7f, -10.8f, 0.86f, 0.52f);
    drawPothole(31.8f, 14.3f, 0.58f, 0.42f);

    // Dust patches near bus stops and road shoulders.
    drawRoadDustPatch(-5.0f, -10.5f, 8.0f, 1.25f, 0.14f);
    drawRoadDustPatch( 5.0f, -10.5f, 8.0f, 1.25f, 0.14f);
    drawRoadDustPatch(-22.2f, 0.0f, 0.45f, 38.0f, 0.10f);
    drawRoadDustPatch( 22.2f, 0.0f, 0.45f, 38.0f, 0.10f);

    // Extra edge lines / shoulders.
    for (float z = -19.5f; z <= 19.5f; z += 4.8f) {
        drawRoadEdgeLine(-21.55f, z, 0.12f, 2.1f);
        drawRoadEdgeLine( 21.55f, z, 0.12f, 2.1f);
    }
}

void drawRealisticRoadSystem(){
    // Raised sidewalk edges
    drawSidewalk(-28,0,1.8f,80);
    drawSidewalk(28,0,1.8f,80);
    drawSidewalk(0,-28,60,1.8f);
    drawSidewalk(0,28,60,1.8f);

    // Asphalt shoulder lines
    drawRoadShoulder(-22,0,0.15f,70);
    drawRoadShoulder(22,0,0.15f,70);
    drawRoadShoulder(0,-22,50,0.15f);
    drawRoadShoulder(0,22,50,0.15f);

    // Lane separators are drawn only on actual road surfaces below. The old
    // highway-style lines crossed the terminal plaza and looked unrealistic.

    // Zebra crossings
    drawCrosswalk(0,-22,true);
    drawCrosswalk(0,22,true);
    drawCrosswalk(-22,0,false);
    drawCrosswalk(22,0,false);

    // Bus stop areas
    drawBusStopMarking(-5,-11);
    drawBusStopMarking(5,-11);

    // Parking slots
    drawParkingLane(-34,-10,6,12);
    drawParkingLane(34,10,6,12);
    drawParkingLane(-34,  4,6,12);
    drawParkingLane(34,-4,6,12);

    // Road signs
    drawTrafficRoadSign(-25,-25);
    drawTrafficRoadSign(25,25);

    // Asphalt crack details
    drawRoadCracks(-8,-15);
    drawRoadCracks(10,12,true);
    drawRoadCracks(-20,7,true);
    drawRoadCracks(14,-8);

    // V64 CRITICAL FIX 2: yellow box-junction hazard markings at the four
    // signal-controlled corners (same coordinates as drawAnimatedTrafficLights()).
    drawBoxJunctionCross(-22.0f,-20.0f, 4.5f);
    drawBoxJunctionCross( 22.0f,-20.0f, 4.5f);
    drawBoxJunctionCross(-22.0f, 20.0f, 4.5f);
    drawBoxJunctionCross( 22.0f, 20.0f, 4.5f);
}

void drawGroundAndRoads() {
    // Literal final colours: road = RGB(0.06, 0.06, 0.07) deep asphalt
    // charcoal-black, ground = neutral charcoal. These values are identical
    // when dayNightBlend is 0.0 or 1.0, and (V64) can no longer be culled
    // away by glCullFace(GL_BACK) regardless of camera angle.
    darkGroundQuadXZ(-95, -90, 95, 90, -0.30f, 70, 66, 0.055f);
    texturedQuadXZ(-88, -82, 88, 82, -0.285f, texRoad, 55, 52);

    // Slightly lighter charcoal foundations separate blocks from black roads.
    darkGroundQuadXZ(-40, -34, -22, 34, -0.10f, 8, 20, 0.085f);
    darkGroundQuadXZ( 22, -34,  40, 34, -0.10f, 8, 20, 0.085f);
    darkGroundQuadXZ(-24, -36,  24,-20, -0.10f,18,  6, 0.085f);
    darkGroundQuadXZ(-24,  20,  24, 36, -0.10f,18,  6, 0.085f);

    // Small landscaped corner pockets with grass tufts instead of flat green rectangles.
    drawLandscapePatch(-39, -35, -30, -26);
    drawLandscapePatch( 30, -35,  39, -26);
    drawLandscapePatch(-39,  26, -30,  35);
    drawLandscapePatch( 30,  26,  39,  35);
    drawDividerPlantDust();

    // Four road segments form a loop around the terminal block.
    texturedQuadXZ(-22, -20, -12, 20, 0.00f, texRoad, 4, 14); // left
    texturedQuadXZ( 12, -20,  22, 20, 0.00f, texRoad, 4, 14); // right
    texturedQuadXZ(-22, -20,  22,-12, 0.00f, texRoad, 15, 3); // front
    texturedQuadXZ(-22,  12,  22, 20, 0.00f, texRoad, 15, 3); // back

    // City-side roads beyond the terminal loop.
    texturedQuadXZ(-32, -24, -22, 24, 0.00f, texRoad, 4, 14);
    texturedQuadXZ( 22, -24,  32, 24, 0.00f, texRoad, 4, 14);
    texturedQuadXZ(-24, -32,  24,-20, 0.00f, texRoad, 16, 4);
    texturedQuadXZ(-24,  20,  24, 32, 0.00f, texRoad, 16, 4);

    // Extra cross-city connectors give the environment a more real urban layout.
    texturedQuadXZ(-58, -24, -32, -14, 0.00f, texRoad, 10, 4);
    texturedQuadXZ( 32,  10,  58,  24, 0.00f, texRoad, 10, 4);
    texturedQuadXZ(-56,  30,  56,  44, 0.00f, texRoad, 24, 4);

    // Central island / terminal plaza remains readable but is still blackish.
    darkGroundQuadXZ(-12, -12, 12, 12, 0.015f, 12, 12, 0.11f);

    drawCurbs();

    // Inner-loop road markings.
    drawDashedLineZ(-17.0f, -19.5f, 19.5f, 2.4f, 1.5f, 0.12f, true);
    drawDashedLineZ( 17.0f, -19.5f, 19.5f, 2.4f, 1.5f, 0.12f, true);
    drawDashedLineZ(-14.4f, -19.5f, 19.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineZ( 14.4f, -19.5f, 19.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineZ(-19.6f, -19.5f, 19.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineZ( 19.6f, -19.5f, 19.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineX(-16.0f, -21.5f, 21.5f, 2.4f, 1.5f, 0.12f, true);
    drawDashedLineX( 16.0f, -21.5f, 21.5f, 2.4f, 1.5f, 0.12f, true);
    drawDashedLineX(-13.5f, -21.5f, 21.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineX(-18.5f, -21.5f, 21.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineX( 13.5f, -21.5f, 21.5f, 2.0f, 1.8f, 0.08f, false);
    drawDashedLineX( 18.5f, -21.5f, 21.5f, 2.0f, 1.8f, 0.08f, false);

    // Yellow centre lines on every outer/city vehicle road.
    drawDashedLineZ(-27.0f, -23.0f, 23.0f, 2.1f, 1.9f, 0.12f, true);
    drawDashedLineZ( 27.0f, -23.0f, 23.0f, 2.1f, 1.9f, 0.12f, true);
    drawDashedLineX(-23.0f, -24.0f, 24.0f, 2.1f, 1.9f, 0.12f, true);
    drawDashedLineX( 23.0f, -24.0f, 24.0f, 2.1f, 1.9f, 0.12f, true);

    // Centre lines for all three cross-city connector roads.
    drawDashedLineX(-19.0f, -57.5f,-32.5f, 2.1f, 1.9f, 0.12f, true);
    drawDashedLineX( 17.0f,  32.5f, 57.5f, 2.1f, 1.9f, 0.12f, true);
    drawDashedLineX( 37.0f, -55.5f, 55.5f, 2.1f, 1.9f, 0.12f, true);

    // Pedestrian crossings.
    drawCrosswalk(0.0f, -14.2f, true);
    drawCrosswalk(13.8f, 2.5f, false);
    drawCrosswalk(-13.8f, 2.5f, false);

    // V8 realistic road enhancements
    drawRealisticRoadSystem();
    drawExtraRoadDetail();
}

// ============================================================================
// Scene: bus
// ============================================================================


void drawWheel() {
    glPushMatrix();

    // Tire wall and tread.
    setMaterial(0.03f, 0.03f, 0.04f, 18, 0.22f);
    torus(0.20f, 0.46f, 20, 32);

    // Inner dark ring.
    setMaterial(0.08f, 0.08f, 0.09f, 28, 0.28f);
    torus(0.05f, 0.23f, 12, 24);

    // Metallic rim.
    setMaterial(0.74f, 0.76f, 0.80f, 110, 1.0f);
    torus(0.035f, 0.24f, 14, 26);
    sphere(0.13f, 18, 12);

    // Spokes.
    setMaterial(0.68f, 0.71f, 0.76f, 105, 1.0f);
    for (int i = 0; i < 6; ++i) {
        glPushMatrix();
        glRotatef(i * 60.0f, 0, 0, 1);
        glTranslatef(0.17f, 0.0f, 0.0f);
        cube(0.22f, 0.045f, 0.045f);
        glPopMatrix();
    }

    // Center cap.
    setMaterial(0.26f, 0.27f, 0.30f, 82, 0.92f);
    glPushMatrix(); glTranslatef(0, 0, 0.085f); sphere(0.05f, 12, 10); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0,-0.085f); sphere(0.05f, 12, 10); glPopMatrix();

    glPopMatrix();
}

void drawBusRouteDisplay(float x, float y, float z, float width = 1.12f) {
    setMaterial(0.04f, 0.05f, 0.05f, 100, 0.95f);
    glPushMatrix(); glTranslatef(x, y, z); cube(0.05f, 0.34f, width); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(1.00f, 0.78f, 0.14f);
    for (float zz = -width * 0.30f; zz <= width * 0.30f; zz += width * 0.13f) {
        glPushMatrix(); glTranslatef(x + 0.03f, y + 0.04f, z + zz); cube(0.01f, 0.07f, width * 0.08f); glPopMatrix();
    }
    glPushMatrix(); glTranslatef(x + 0.03f, y - 0.05f, z); cube(0.01f, 0.04f, width * 0.52f); glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawBusGlassPane(float x, float y, float z, float sx, float sy, float sz) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    setPBRMaterial(0.05f, 0.08f, 0.10f, 0.10f, 0.10f, 0.22f);
    glPushMatrix(); glTranslatef(x,y,z); cube(sx,sy,sz); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor4f(0.78f,0.88f,0.98f,0.14f);
    glPushMatrix(); glTranslatef(x + sx*0.02f, y + sy*0.16f, z); cube(0.008f, sy*0.55f, sz*0.48f); glPopMatrix();
    glColor4f(1.0f,1.0f,1.0f,0.08f);
    glPushMatrix(); glTranslatef(x + sx*0.01f, y + sy*0.32f, z); cube(0.008f, sy*0.18f, sz*0.42f); glPopMatrix();
    glEnable(GL_LIGHTING);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawBusInteriorSeats(float xStart, float xEnd) {
    // Left row seats.
    setMaterial(0.10f,0.34f,0.62f,42,0.42f);
    for (float x = xStart; x <= xEnd; x += 0.62f) {
        glPushMatrix(); glTranslatef(x,1.05f, 0.40f); cube(0.28f,0.12f,0.24f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,1.28f, 0.48f); cube(0.28f,0.32f,0.08f); glPopMatrix();
    }
    // Right row seats.
    for (float x = xStart; x <= xEnd; x += 0.62f) {
        glPushMatrix(); glTranslatef(x,1.05f,-0.40f); cube(0.28f,0.12f,0.24f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,1.28f,-0.48f); cube(0.28f,0.32f,0.08f); glPopMatrix();
    }

    // Floor and aisle.
    setMaterial(0.22f,0.23f,0.25f,28,0.22f);
    glPushMatrix(); glTranslatef((xStart+xEnd)*0.5f,0.86f,0.0f); cube((xEnd-xStart)+0.46f,0.08f,1.18f); glPopMatrix();
}

void drawBusDoorSet(float xCenter, float zSide = -1.05f, bool animated = false) {
    setPBRMaterial(0.10f,0.11f,0.12f,0.55f,0.28f);
    glPushMatrix(); glTranslatef(xCenter,1.42f,zSide); cube(0.88f,1.70f,0.04f); glPopMatrix();

    float open = animated ? (busDoorAngle / 70.0f) * 0.18f : 0.0f;
    drawBusGlassPane(xCenter - 0.21f - open, 1.46f, zSide - 0.01f, 0.34f, 0.86f, 0.015f);
    drawBusGlassPane(xCenter + 0.21f + open, 1.46f, zSide - 0.01f, 0.34f, 0.86f, 0.015f);
    setPBRMaterial(0.72f,0.74f,0.78f,0.72f,0.18f);
    glPushMatrix(); glTranslatef(xCenter,1.46f,zSide-0.02f); cube(0.025f,0.90f,0.018f); glPopMatrix();
}

void drawBusLiveryStripe(float xCenter, float length, float zSide, float r, float g, float b) {
    // White body panel for modern city-transport look.
    setPBRMaterial(0.92f,0.94f,0.96f,0.18f,0.30f);
    glPushMatrix(); glTranslatef(xCenter,1.58f,zSide); cube(length,0.76f,0.03f); glPopMatrix();

    // Main colored band.
    setPBRMaterial(r,g,b,0.42f,0.34f);
    glPushMatrix(); glTranslatef(xCenter - 0.08f,1.12f,zSide); cube(length*0.95f,0.20f,0.03f); glPopMatrix();

    // Lower metallic accent.
    setPBRMaterial(0.74f,0.76f,0.80f,0.88f,0.16f);
    glPushMatrix(); glTranslatef(xCenter,0.72f,zSide); cube(length*0.98f,0.05f,0.025f); glPopMatrix();

    // Diagonal graphic accent toward the front.
    setMaterial(std::min(1.0f, r + 0.20f), std::min(1.0f, g + 0.18f), std::min(1.0f, b + 0.10f), 88, 0.92f);
    glPushMatrix(); glTranslatef(xCenter + length*0.18f,1.18f,zSide); glRotatef(-22.0f,0,0,1); cube(length*0.22f,0.36f,0.028f); glPopMatrix();
}

void drawBusInteriorHandrails(float length);
void drawRearDoor(float x, float zSide=-1.05f);

void drawBusModuleBody(float length, bool frontModule, float r, float g, float b) {
    // Modern city bus body: light upper shell, colored lower body and dark glass band.
    setPBRMaterial(0.95f,0.96f,0.97f,0.16f,0.30f);
    glPushMatrix(); glTranslatef(0.0f,1.15f,0.0f); cube(length,1.22f,2.02f); glPopMatrix();

    // Main color skirt similar to modern transport buses.
    setPBRMaterial(r,g,b,0.48f,0.36f);
    glPushMatrix(); glTranslatef(0.0f,0.65f,0.0f); cube(length*0.99f,0.42f,2.06f); glPopMatrix();

    // Window belt / dark mask.
    setPBRMaterial(0.08f,0.09f,0.10f,0.78f,0.18f);
    glPushMatrix(); glTranslatef(0.0f,2.01f,0.0f); cube(length*0.93f,0.60f,1.88f); glPopMatrix();

    // Slim white roof shoulder.
    setPBRMaterial(0.96f,0.97f,0.98f,0.18f,0.32f);
    glPushMatrix(); glTranslatef(0.0f,2.43f,0.0f); cube(length*0.91f,0.22f,1.82f); glPopMatrix();

    // Roof cap and AC pod.
    setPBRMaterial(0.18f,0.19f,0.21f,0.82f,0.22f);
    glPushMatrix(); glTranslatef(0.0f,2.74f,0.0f); cube(length*0.90f,0.11f,1.76f); glPopMatrix();
    setPBRMaterial(0.28f,0.30f,0.32f,0.74f,0.22f);
    glPushMatrix(); glTranslatef(0.0f,2.88f,0.0f); cube(length*0.34f,0.18f,0.78f); glPopMatrix();
    glPushMatrix(); glTranslatef(-length*0.22f,2.84f,0.0f); cube(0.58f,0.12f,0.70f); glPopMatrix();
    glPushMatrix(); glTranslatef( length*0.22f,2.84f,0.0f); cube(0.58f,0.12f,0.70f); glPopMatrix();

    drawBusInteriorSeats(-length*0.35f, length*0.28f);
    drawBusInteriorHandrails(length);

    // Side livery panels / graphics.
    drawBusLiveryStripe(0.0f, length*0.92f,  1.02f, r,g,b);
    drawBusLiveryStripe(0.0f, length*0.92f, -1.02f, r,g,b);

    // Side windows.
    for (float x = -length*0.34f; x <= length*0.28f; x += 0.62f) {
        if (!(frontModule && x > 0.30f && x < 1.10f)) {
            drawBusGlassPane(x, 2.01f, 1.025f, 0.46f, 0.58f, 0.015f);
        }
    }
    for (float x = -length*0.34f; x <= length*0.28f; x += 0.62f) {
        if (frontModule && x > 0.18f && x < 1.08f) continue;
        drawBusGlassPane(x, 2.01f,-1.025f, 0.46f, 0.58f, 0.015f);
    }

    // Wheel arch trim.
    setPBRMaterial(0.12f,0.13f,0.14f,0.58f,0.28f);
    for (float x : {-length*0.28f, length*0.28f}) {
        glPushMatrix(); glTranslatef(x,0.82f, 1.00f); cube(0.78f,0.17f,0.10f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,0.82f,-1.00f); cube(0.78f,0.17f,0.10f); glPopMatrix();
    }
}

void drawModernFrontCab(float r, float g, float b, bool animatedDoor = true) {
    // Modern flat-front bus inspired by the reference image.
    setPBRMaterial(r,g,b,0.48f,0.34f);
    glPushMatrix(); glTranslatef(2.44f,1.26f,0.0f); cube(0.96f,1.30f,1.94f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.90f,1.54f,0.0f); cube(0.24f,0.86f,1.76f); glPopMatrix();

    // Black front glass mask.
    setPBRMaterial(0.05f,0.06f,0.07f,0.82f,0.14f);
    glPushMatrix(); glTranslatef(2.90f,1.95f,0.0f); cube(0.06f,1.24f,1.56f); glPopMatrix();
    drawBusGlassPane(2.92f,1.98f,0.0f, 0.018f, 1.10f, 1.42f);
    drawBusGlassPane(2.78f,1.94f,0.50f, 0.018f, 0.86f, 0.36f);
    drawBusGlassPane(2.78f,1.94f,-0.50f,0.018f, 0.86f, 0.36f);

    // Driver quarter glass.
    drawBusGlassPane(2.24f,1.98f,1.02f,0.28f,0.52f,0.015f);
    drawBusGlassPane(2.24f,1.98f,-1.02f,0.28f,0.52f,0.015f);

    // White lower light signatures like a real modern bus.
    setPBRMaterial(0.97f,0.98f,0.99f,0.18f,0.30f);
    glPushMatrix(); glTranslatef(2.98f,0.82f, 0.60f); cube(0.05f,0.11f,0.34f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.98f,0.82f,-0.60f); cube(0.05f,0.11f,0.34f); glPopMatrix();

    // Lower bumper lip.
    setPBRMaterial(0.20f,0.21f,0.23f,0.72f,0.18f);
    glPushMatrix(); glTranslatef(2.98f,0.56f,0.0f); cube(0.06f,0.16f,1.24f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.82f,0.46f,0.0f); cube(0.36f,0.10f,0.68f); glPopMatrix();

    // Front route display.
    drawBusRouteDisplay(2.98f, 2.46f, 0.0f, 1.18f);

    // Headlights / DRL glow.
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.98f,0.84f);
    glPushMatrix(); glTranslatef(3.05f,1.02f, 0.62f); cube(0.03f,0.08f,0.24f); glPopMatrix();
    glPushMatrix(); glTranslatef(3.05f,1.02f,-0.62f); cube(0.03f,0.08f,0.24f); glPopMatrix();
    glColor3f(0.82f,0.94f,1.0f);
    glPushMatrix(); glTranslatef(3.05f,0.85f, 0.60f); cube(0.03f,0.04f,0.28f); glPopMatrix();
    glPushMatrix(); glTranslatef(3.05f,0.85f,-0.60f); cube(0.03f,0.04f,0.28f); glPopMatrix();
    glEnable(GL_LIGHTING);

    // Mirrors.
    setPBRMaterial(0.06f,0.06f,0.07f,0.72f,0.14f);
    glPushMatrix(); glTranslatef(2.58f,2.10f, 1.17f); cube(0.08f,0.32f,0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.58f,2.10f,-1.17f); cube(0.08f,0.32f,0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.80f,2.18f, 1.28f); cube(0.28f,0.16f,0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.80f,2.18f,-1.28f); cube(0.28f,0.16f,0.08f); glPopMatrix();

    // Front passenger door.
    drawBusDoorSet(1.32f, -1.05f, animatedDoor);
}

// V13 interior details: handrails and standing poles visible through glass.
void drawBusInteriorHandrails(float length) {
    setMaterial(0.86f,0.88f,0.90f,95,1.0f);

    // Long ceiling rails.
    glPushMatrix();
    glTranslatef(0.0f,2.38f,0.48f);
    glRotatef(90,0,1,0);
    cylinder(0.025f,length*0.72f,12);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f,2.38f,-0.48f);
    glRotatef(90,0,1,0);
    cylinder(0.025f,length*0.72f,12);
    glPopMatrix();

    // Vertical standing passenger poles.
    for(float x=-length*0.30f;x<=length*0.25f;x+=0.65f){
        glPushMatrix();
        glTranslatef(x,1.55f,0.0f);
        glRotatef(-90,1,0,0);
        cylinder(0.022f,1.15f,12);
        glPopMatrix();
    }
}

void drawRearDoor(float x, float zSide){
    setMaterial(0.08f,0.10f,0.12f,85,0.95f);
    glPushMatrix();
    glTranslatef(x,1.35f,zSide);
    cube(0.95f,1.72f,0.04f);
    glPopMatrix();

    drawBusGlassPane(x-0.22f,1.45f,zSide-0.03f,0.34f,0.85f,0.015f);
    drawBusGlassPane(x+0.22f,1.45f,zSide-0.03f,0.34f,0.85f,0.015f);

    setMaterial(0.75f,0.77f,0.80f,100,1.0f);
    glPushMatrix();
    glTranslatef(x,1.45f,zSide-0.05f);
    cube(0.025f,0.90f,0.02f);
    glPopMatrix();
}

void drawBusWheelPair(float x, bool rotateWheels = true) {
    for (float z : {1.08f, -1.08f}) {
        glPushMatrix();
        glTranslatef(x,0.55f,z);
        glRotatef(rotateWheels ? busWheelRotation : 0.0f, 0,0,1);
        drawWheel();
        glPopMatrix();
    }
}

void drawArticulatedConnector(float bendAngle) {
    glPushMatrix();
    // Half of the rear-module steering angle keeps the bellows visually
    // centred between the front and rear sections.
    glRotatef(bendAngle * 0.5f,0,1,0);

    // Dark connector housing.
    setMaterial(0.18f,0.19f,0.20f,42,0.35f);
    glPushMatrix(); glTranslatef(-0.10f,1.52f,0.0f); cube(0.72f,1.38f,1.82f); glPopMatrix();
    setMaterial(0.13f,0.14f,0.15f,24,0.22f);
    for (float x=-0.26f; x<=0.26f; x+=0.10f) {
        glPushMatrix(); glTranslatef(x,1.52f,0.93f); cube(0.03f,1.30f,0.08f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,1.52f,-0.93f); cube(0.03f,1.30f,0.08f); glPopMatrix();
    }
    // Bellows folds.
    for (float x=-0.26f; x<=0.26f; x+=0.10f) {
        glPushMatrix(); glTranslatef(x,2.12f,0.0f); cube(0.03f,0.06f,1.72f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,0.96f,0.0f); cube(0.03f,0.06f,1.72f); glPopMatrix();
    }

    // Joint plate hint.
    setMaterial(0.32f,0.33f,0.35f,64,0.90f);
    glPushMatrix(); glTranslatef(-0.08f,0.86f,0.0f); cube(0.60f,0.08f,1.15f); glPopMatrix();
    glPopMatrix();
}

// Dynamic light overlay kept outside static display lists. Standard alpha
// blending and deliberately low opacity make this read as light ON black road,
// never as a replacement road colour.
void drawBusNightOverlay() {
    float night = clampf((dayNightBlend - 0.20f) / 0.80f, 0.0f, 1.0f);
    if (night <= 0.001f) return;

    drawLightPool(4.15f, 0.64f, 1.08f, 1.0f, 0.90f, 0.55f, 0.055f*night);
    drawLightPool(4.15f,-0.64f, 1.08f, 1.0f, 0.90f, 0.55f, 0.055f*night);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(1.0f, 0.95f, 0.74f, 0.075f*night);
    glBegin(GL_TRIANGLES);
        glVertex3f(3.02f,1.12f, 0.64f); glVertex3f(5.85f,0.28f, 0.14f); glVertex3f(5.85f,1.10f, 1.04f);
        glVertex3f(3.02f,1.12f,-0.64f); glVertex3f(5.85f,0.28f,-1.04f); glVertex3f(5.85f,1.10f,-0.14f);
    glEnd();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawBus(float r = 0.05f, float g = 0.72f, float b = 0.13f,
             bool liveEffects = true) {
    glPushMatrix();

    // Longer articulated-bus shadow.
    drawSoftShadow(-0.25f, 0.0f, 4.95f, 1.42f, 0.21f);

    // Front module.
    glPushMatrix();
    glTranslatef(1.10f,0.0f,0.0f);
    drawBusModuleBody(4.25f, true, r,g,b);
    drawModernFrontCab(r,g,b,liveEffects);
    drawDriverCabin();
    drawBusWheelPair(-0.96f,liveEffects);
    drawBusWheelPair( 1.55f,liveEffects);
    glPopMatrix();

    // Articulation joint.
    float bendAngle = (liveEffects && animationOn)
        ? 6.0f * std::sin(busWheelRotation * PI / 180.0f * 0.30f) : 0.0f;
    glPushMatrix();
    glTranslatef(-1.18f,0.0f,0.0f);
    drawArticulatedConnector(bendAngle);
    glPopMatrix();

    // Rear module rotates slightly around joint.
    glPushMatrix();
    glTranslatef(-1.45f,0.0f,0.0f);
    glRotatef(bendAngle,0,1,0);
    glTranslatef(-2.20f,0.0f,0.0f);
    drawBusModuleBody(3.80f, false, r,g,b);
    // Rear door for articulated section.
    drawBusDoorSet(0.45f, -1.05f, liveEffects);

    // Rear back cap.
    setMaterial(0.96f,0.97f,0.98f,60,0.45f);
    glPushMatrix(); glTranslatef(-2.10f,1.58f,0.0f); cube(0.32f,1.16f,1.76f); glPopMatrix();
    setMaterial(r,g,b,78,0.86f);
    glPushMatrix(); glTranslatef(-2.10f,0.78f,0.0f); cube(0.30f,0.36f,1.84f); glPopMatrix();

    // Rear window and vent strip.
    drawBusGlassPane(-2.18f,1.98f,0.0f,0.018f,0.84f,1.38f);
    drawRearDoor(-0.95f, -1.05f);
    setMaterial(0.22f,0.24f,0.26f,58,0.80f);
    glPushMatrix(); glTranslatef(-1.00f,2.84f,0.0f); cube(1.80f,0.08f,0.18f); glPopMatrix();

    // Rear tail lights.
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.06f,0.05f);
    glPushMatrix(); glTranslatef(-2.30f,1.08f, 0.68f); cube(0.03f,0.18f,0.16f); glPopMatrix();
    glPushMatrix(); glTranslatef(-2.30f,1.08f,-0.68f); cube(0.03f,0.18f,0.16f); glPopMatrix();
    glColor3f(1.0f,0.76f,0.12f);
    glPushMatrix(); glTranslatef(-2.30f,0.84f, 0.68f); cube(0.03f,0.08f,0.16f); glPopMatrix();
    glPushMatrix(); glTranslatef(-2.30f,0.84f,-0.68f); cube(0.03f,0.08f,0.16f); glPopMatrix();
    glEnable(GL_LIGHTING);

    // Rear axle wheel pair.
    drawBusWheelPair(-0.20f,liveEffects);
    drawBusWheelPair( 1.35f,liveEffects);
    glPopMatrix();

    // Animated turn indicators / hazard lights.
    bool indicatorOn = std::fmod(indicatorBlinkTimer, 1.0f) < 0.52f;
    if (liveEffects && indicatorOn) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f,0.62f,0.10f);
        // Front and side markers
        glPushMatrix(); glTranslatef(3.06f,1.34f, 0.92f); cube(0.02f,0.10f,0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef(3.06f,1.34f,-0.92f); cube(0.02f,0.10f,0.12f); glPopMatrix();
        glPushMatrix(); glTranslatef(1.40f,1.06f, 1.05f); cube(0.18f,0.08f,0.02f); glPopMatrix();
        glPushMatrix(); glTranslatef(1.40f,1.06f,-1.05f); cube(0.18f,0.08f,0.02f); glPopMatrix();
        glPushMatrix(); glTranslatef(-2.34f,1.18f, 0.76f); cube(0.02f,0.10f,0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef(-2.34f,1.18f,-0.76f); cube(0.02f,0.10f,0.14f); glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    if (liveEffects) drawBusNightOverlay();

    glPopMatrix();
}



void drawMiniBusLights(float brake, float indicator) {
    // V32 realistic brake and indicator lights.
    glDisable(GL_LIGHTING);

    // Brake lights
    glColor3f(1.0f, 0.05f, 0.03f);
    glPushMatrix();
    glTranslatef(-1.96f,1.02f,0.66f);
    cube(0.035f,0.12f,0.14f * (1.0f + brake));
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.96f,1.02f,-0.66f);
    cube(0.035f,0.12f,0.14f * (1.0f + brake));
    glPopMatrix();

    // Indicators
    glColor3f(1.0f,0.65f,0.05f);
    float blink = (indicator > 0.5f) ? 1.0f : 0.25f;

    glPushMatrix();
    glTranslatef(1.90f,0.95f,0.72f);
    cube(0.025f,0.06f,0.12f * blink);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.90f,0.95f,-0.72f);
    cube(0.025f,0.06f,0.12f * blink);
    glPopMatrix();

    glEnable(GL_LIGHTING);
}

void drawMiniBusRoundWheelPair(float originalX, bool rotateWheels) {
    // Body scale is (0.74, 0.88, 0.86). Apply those factors only to wheel
    // POSITIONS, then use one uniform 0.81 scale for the circular geometry.
    // Therefore torus X/Y radii remain equal and the wheels cannot become oval.
    for (float originalZ : {1.08f, -1.08f}) {
        glPushMatrix();
        glTranslatef(originalX*0.74f, 0.55f*0.88f, originalZ*0.86f);
        glRotatef(rotateWheels ? busWheelRotation : 0.0f, 0,0,1);
        glScalef(0.81f,0.81f,0.81f);
        drawWheel();
        glPopMatrix();
    }
}

void drawMiniBusNightOverlay() {
    float night = clampf((dayNightBlend - 0.20f) / 0.80f, 0.0f, 1.0f);
    if (night <= 0.001f) return;
    drawLightPool(2.72f, 0.50f, 0.72f, 1.0f,0.91f,0.60f,0.045f*night);
    drawLightPool(2.72f,-0.50f, 0.72f, 1.0f,0.91f,0.60f,0.045f*night);
}

void drawMiniBus(float r, float g, float b, bool liveEffects) {
    glPushMatrix();

    // V31: more realistic compact minibus proportion.
    drawSoftShadow(0.08f, 0.0f, 2.82f, 1.02f, 0.16f);

    glPushMatrix();
    glScalef(0.74f, 0.88f, 0.86f);

    // Slightly longer body but slimmer width for a realistic lane fit.
    drawBusModuleBody(3.45f, true, r, g, b);
    drawModernFrontCab(r, g, b, liveEffects);

    // Rear end cap.
    setPBRMaterial(0.95f,0.96f,0.97f,0.14f,0.30f);
    glPushMatrix(); glTranslatef(-1.78f,1.56f,0.0f); cube(0.24f,1.14f,1.70f); glPopMatrix();
    setPBRMaterial(r,g,b,0.46f,0.34f);
    glPushMatrix(); glTranslatef(-1.78f,0.68f,0.0f); cube(0.22f,0.38f,1.80f); glPopMatrix();

    // Rear window.
    drawBusGlassPane(-1.86f,1.98f,0.0f,0.018f,0.80f,1.30f);

    // Rear lights.
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.08f,0.06f);
    glPushMatrix(); glTranslatef(-1.95f,1.02f, 0.64f); cube(0.03f,0.16f,0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.95f,1.02f,-0.64f); cube(0.03f,0.16f,0.14f); glPopMatrix();
    glColor3f(1.0f,0.76f,0.12f);
    glPushMatrix(); glTranslatef(-1.95f,0.80f, 0.64f); cube(0.03f,0.08f,0.14f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.95f,0.80f,-0.64f); cube(0.03f,0.08f,0.14f); glPopMatrix();
    glEnable(GL_LIGHTING);

    drawMiniBusLights(liveEffects ? trafficBrakePulse : 0.0f,
                      liveEffects ? indicatorPulse : 0.0f);

    glPopMatrix();

    // IMPORTANT: wheels are outside the non-uniform body scale above.
    drawMiniBusRoundWheelPair(-0.78f,liveEffects);
    drawMiniBusRoundWheelPair( 1.28f,liveEffects);
    if (liveEffects) drawMiniBusNightOverlay();
    glPopMatrix();
}

void drawCar(float r = 0.78f, float g = 0.80f, float b = 0.83f) {
    glPushMatrix();

    drawSoftShadow(0.0f, 0.0f, 1.65f, 0.95f, 0.15f);

    setMaterial(r,g,b,60,0.78f);
    glPushMatrix(); glTranslatef(0,0.62f,0); cube(2.65f,0.72f,1.34f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.15f,1.14f,0); cube(1.55f,0.50f,1.18f); glPopMatrix();

    setMaterial(0.10f,0.14f,0.18f,95,1.0f);
    glPushMatrix(); glTranslatef(-0.15f,1.17f,0.60f); cube(1.20f,0.36f,0.03f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.15f,1.17f,-0.60f); cube(1.20f,0.36f,0.03f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.92f,0.92f,0); cube(0.03f,0.40f,1.00f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.22f,0.92f,0); cube(0.03f,0.30f,0.92f); glPopMatrix();

    setMaterial(0.08f,0.08f,0.09f,45,0.60f);
    glPushMatrix(); glTranslatef( 1.35f,0.42f,0); cube(0.12f,0.20f,1.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.35f,0.42f,0); cube(0.12f,0.20f,1.18f); glPopMatrix();

    for (float x : {-0.90f, 0.92f}) {
        glPushMatrix(); glTranslatef(x,0.24f, 0.76f); glRotatef(90,1,0,0); drawWheel(); glPopMatrix();
        glPushMatrix(); glTranslatef(x,0.24f,-0.76f); glRotatef(90,1,0,0); drawWheel(); glPopMatrix();
    }

    // V67: simple procedural side accent stripe (a darker shade of whatever
    // body colour was passed in) so a plain-coloured car/taxi reads as a
    // liveried vehicle rather than one flat block - works for any r,g,b.
    setMaterial(r*0.45f, g*0.45f, b*0.45f, 40, 0.5f);
    glPushMatrix(); glTranslatef(-0.05f, 0.66f, 0.685f); cube(2.35f, 0.09f, 0.02f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.05f, 0.66f, -0.685f); cube(2.35f, 0.09f, 0.02f); glPopMatrix();

    if (nightMode) glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.96f,0.70f);
    glPushMatrix(); glTranslatef(1.39f,0.75f, 0.42f); sphere(0.08f,12,8); glPopMatrix();
    glPushMatrix(); glTranslatef(1.39f,0.75f,-0.42f); sphere(0.08f,12,8); glPopMatrix();
    glColor3f(0.95f,0.12f,0.08f);
    glPushMatrix(); glTranslatef(-1.39f,0.75f, 0.42f); sphere(0.06f,12,8); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.39f,0.75f,-0.42f); sphere(0.06f,12,8); glPopMatrix();
    if (nightMode) glEnable(GL_LIGHTING);

    if (nightMode) {
        drawLightPool(2.15f, 0.45f, 0.55f, 1.0f, 0.94f, 0.70f, 0.10f);
        drawLightPool(2.15f,-0.45f, 0.55f, 1.0f, 0.94f, 0.70f, 0.10f);
    }

    glPopMatrix();
}


void drawTaxi(float r, float g, float b) {
    glPushMatrix();
    drawCar(r,g,b);
    setPBRMaterial(0.96f,0.96f,0.96f,0.12f,0.38f);
    glPushMatrix(); glTranslatef(0.00f,1.52f,0.0f); cube(0.52f,0.18f,0.38f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.05f,0.05f,0.05f);
    glPushMatrix(); glTranslatef(0.00f,1.56f,0.0f); cube(0.22f,0.04f,0.22f); glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawMotorcycle(float r, float g, float b) {
    glPushMatrix();
    drawSoftShadow(0.0f, 0.0f, 0.95f, 0.52f, 0.12f);

    setPBRMaterial(0.08f,0.08f,0.09f,0.65f,0.18f);
    glPushMatrix(); glTranslatef(-0.62f,0.22f,0.0f); glRotatef(90,1,0,0); glScalef(0.68f,0.68f,0.68f); drawWheel(); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.62f,0.22f,0.0f); glRotatef(90,1,0,0); glScalef(0.68f,0.68f,0.68f); drawWheel(); glPopMatrix();

    setPBRMaterial(r,g,b,0.18f,0.32f);
    glPushMatrix(); glTranslatef(0.0f,0.58f,0.0f); cube(1.05f,0.10f,0.16f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.10f,0.75f,0.0f); cube(0.48f,0.16f,0.24f); glPopMatrix();

    // V67: simple white racing-stripe decal on the tank so the motorcycle
    // reads as a liveried vehicle rather than one flat colour block.
    setMaterial(0.95f,0.95f,0.95f,60,0.6f);
    glPushMatrix(); glTranslatef(-0.08f,0.79f,0.0f); cube(0.30f,0.03f,0.10f); glPopMatrix();

    setPBRMaterial(0.18f,0.18f,0.20f,0.75f,0.18f);
    glPushMatrix(); glTranslatef(0.52f,0.92f,0.0f); cube(0.06f,0.42f,0.06f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.60f,1.05f,0.0f); cube(0.34f,0.05f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.34f,0.90f,0.0f); cube(0.05f,0.24f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.38f,1.00f,0.0f); cube(0.24f,0.05f,0.20f); glPopMatrix();

    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.94f,0.70f);
    glPushMatrix(); glTranslatef(0.70f,0.93f,0.0f); sphere(0.06f,10,8); glPopMatrix();
    glColor3f(0.95f,0.12f,0.10f);
    glPushMatrix(); glTranslatef(-0.78f,0.82f,0.0f); sphere(0.05f,10,8); glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// ============================================================================
// Scene: trees, lamps, fence
// ============================================================================

void drawGrassTuft(float x, float z, float scale) {
    glPushMatrix();
    glTranslatef(x, 0.02f, z);
    glScalef(scale, scale, scale);
    setMaterial(0.12f,0.44f,0.14f,16,0.10f);
    for (int i = 0; i < 5; ++i) {
        float off = -0.10f + i * 0.05f;
        glPushMatrix();
        glTranslatef(off, 0.14f + 0.02f * (i % 2), 0.0f);
        glRotatef(-14.0f + i * 7.0f, 0, 0, 1);
        cube(0.028f, 0.28f + 0.03f * (i % 3), 0.022f);
        glPopMatrix();
    }
    setMaterial(0.19f,0.52f,0.16f,16,0.12f);
    for (int i = 0; i < 4; ++i) {
        float off = -0.07f + i * 0.045f;
        glPushMatrix();
        glTranslatef(0.0f, 0.14f, off);
        glRotatef(90, 0, 1, 0);
        glRotatef(-18.0f + i * 11.0f, 0, 0, 1);
        cube(0.026f, 0.24f + 0.03f * (i % 2), 0.020f);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawLandscapePatch(float x1, float z1, float x2, float z2) {
    texturedQuadXZ(x1, z1, x2, z2, -0.198f, texGrass, 4, 4);

    // Perimeter grass tufts for a more alive vegetation feel.
    for (float x = x1 + 0.6f; x < x2 - 0.4f; x += 1.15f) {
        drawGrassTuft(x, z1 + 0.45f, 0.85f);
        drawGrassTuft(x, z2 - 0.45f, 0.78f);
    }
    for (float z = z1 + 0.6f; z < z2 - 0.4f; z += 1.10f) {
        drawGrassTuft(x1 + 0.45f, z, 0.76f);
        drawGrassTuft(x2 - 0.45f, z, 0.82f);
    }

    // Inner random clumps.
    const float cx[] = {0.18f, 0.42f, 0.68f, 0.30f, 0.58f};
    const float cz[] = {0.24f, 0.66f, 0.38f, 0.48f, 0.76f};
    const float sc[] = {0.95f, 1.05f, 0.88f, 0.74f, 0.92f};
    for (int i = 0; i < 5; ++i) {
        float px = x1 + (x2 - x1) * cx[i];
        float pz = z1 + (z2 - z1) * cz[i];
        drawGrassTuft(px, pz, sc[i]);
    }
}

void drawTree(float scale = 1.0f, float swayDeg = 0.0f) {
    glPushMatrix();
    glScalef(scale, scale, scale);

    // V48 mature tropical roadside tree: thicker tapered-looking trunk.
    setMaterial(0.25f,0.17f,0.09f,12,0.10f);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    cylinder(0.25f, 3.15f, 20);
    glPopMatrix();

    // Root flare and darker lower bark.
    setMaterial(0.20f,0.13f,0.07f,10,0.08f);
    glPushMatrix(); glTranslatef(0,0.16f,0); glScalef(1.0f,0.55f,1.0f); sphere(0.38f,16,10); glPopMatrix();

    // V67: gentle wind sway - only the branches+canopy tilt (a real trunk is
    // rigid; only the crown moves in a breeze). Pivoting at the trunk base
    // with a small angle on two axes gives a believable, cheap "breathing"
    // motion without any shader.
    glPushMatrix();
    glRotatef(swayDeg, 1.0f, 0.0f, 0.35f);

    // Forked spreading branches form the broad rain-tree silhouette seen in Dhaka.
    for (int i = 0; i < 6; ++i) {
        glPushMatrix();
        glTranslatef(0.0f, 2.05f + 0.18f * (i % 3), 0.0f);
        glRotatef(i * 60.0f + 18.0f, 0, 1, 0);
        glTranslatef(0.48f, 0.28f, 0.0f);
        glRotatef(-58.0f, 0, 0, 1);
        cube(0.13f, 1.25f, 0.13f);
        glPopMatrix();
    }

    // Irregular layered canopy; flattened lobes read as foliage, not spheres.
    const float leafPos[][4] = {
        { 0.00f,3.50f, 0.00f,1.18f}, { 1.05f,3.28f, 0.12f,0.92f},
        {-1.02f,3.30f,-0.10f,0.96f}, { 0.35f,3.62f, 0.88f,0.86f},
        {-0.28f,3.58f,-0.92f,0.84f}, { 1.18f,3.34f,-0.74f,0.72f},
        {-1.20f,3.36f, 0.72f,0.76f}, { 0.00f,4.02f, 0.08f,0.92f},
        { 1.72f,3.14f, 0.05f,0.62f}, {-1.68f,3.18f, 0.02f,0.64f},
        { 0.60f,3.22f,-1.28f,0.58f}, {-0.62f,3.26f, 1.26f,0.60f}
    };
    const float leafCol[][3] = {
        {0.075f,0.25f,0.08f}, {0.10f,0.32f,0.10f},
        {0.13f,0.38f,0.12f}, {0.08f,0.28f,0.07f}
    };
    for (int i = 0; i < 12; ++i) {
        const float* c = leafCol[i % 4];
        setMaterial(c[0], c[1], c[2], 10, 0.06f);
        glPushMatrix();
        glTranslatef(leafPos[i][0], leafPos[i][1], leafPos[i][2]);
        glScalef(1.28f,0.72f,1.02f);
        sphere(leafPos[i][3], 18, 12);
        glPopMatrix();
    }

    glPopMatrix(); // end sway pivot

    glPopMatrix();
}

void drawLampPost(float x, float z, float rotation = 0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotation,0,1,0);

    setMaterial(0.06f,0.065f,0.075f,72,0.90f);
    glPushMatrix(); glRotatef(-90,1,0,0); cylinder(0.095f,4.5f,18); glPopMatrix();
    glPushMatrix(); glTranslatef(0,4.35f,0); cube(1.45f,0.10f,0.10f); glPopMatrix();

    const float night = nightMode
        ? clampf((dayNightBlend-0.10f)/0.90f,0.0f,1.0f) : 0.0f;
    if (night > 0.001f) {
        drawLightPool( 0.62f, 0.00f, 1.20f, 1.0f, 0.86f, 0.55f, 0.052f*night);
        drawLightPool(-0.62f, 0.00f, 1.20f, 1.0f, 0.86f, 0.55f, 0.052f*night);
    }

    if (night > 0.001f) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f,0.92f,0.62f);
    } else {
        setMaterial(0.62f,0.60f,0.48f,35,0.25f);
    }
    glPushMatrix(); glTranslatef( 0.62f,4.20f,0); sphere(0.16f,14,9); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.62f,4.20f,0); sphere(0.16f,14,9); glPopMatrix();

    if (night > 0.001f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.88f, 0.55f, 0.16f*night);
        glPushMatrix(); glTranslatef( 0.62f,4.20f,0); sphere(0.34f,16,10); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.62f,4.20f,0); sphere(0.34f,16,10); glPopMatrix();
        glColor4f(1.0f, 0.88f, 0.55f, 0.06f*night);
        glPushMatrix(); glTranslatef( 0.62f,4.20f,0); sphere(0.52f,16,10); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.62f,4.20f,0); sphere(0.52f,16,10); glPopMatrix();
        glDisable(GL_BLEND);
    }
    if (night > 0.001f) glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawFence() {
    // Keep only a few low urban edge rails at the back service lane so the
    // station feels open to the city instead of fenced like a rural compound.
    setMaterial(0.14f,0.15f,0.16f,50,0.70f);
    for (float x=-20.0f; x<=20.0f; x+=2.0f) {
        glPushMatrix(); glTranslatef(x,0.55f,27.3f); cube(0.08f,1.10f,0.08f); glPopMatrix();
    }
    glPushMatrix(); glTranslatef(0,1.05f,27.3f); cube(40.0f,0.06f,0.06f); glPopMatrix();
}

// ============================================================================
// Scene: bus shelter / platform canopy
// ============================================================================
void drawBench(float length = 4.5f) {
    setMaterial(1,1,1,25,0.35f);
    glPushMatrix(); glTranslatef(0,0.70f,0); texturedBox(length,0.16f,0.55f,texWood,4,1); glPopMatrix();
    glPushMatrix(); glTranslatef(0,1.13f,0.25f); texturedBox(length,0.75f,0.14f,texWood,4,1); glPopMatrix();
    setMaterial(0.08f,0.08f,0.09f,45,0.65f);
    for (float x=-length/2+0.35f; x<=length/2-0.35f; x+=length-0.70f) {
        glPushMatrix(); glTranslatef(x,0.35f,0); cube(0.12f,0.70f,0.12f); glPopMatrix();
    }
}

void drawRouteSign(int number) {
    glPushMatrix();
    setMaterial(0.07f,0.08f,0.10f,55,0.80f);
    glPushMatrix(); glTranslatef(0,1.50f,0); cube(0.10f,3.0f,0.10f); glPopMatrix();
    setMaterial(0.18f,0.22f,0.24f,70,0.85f);
    glPushMatrix(); glTranslatef(0,2.75f,0); cube(0.95f,0.65f,0.12f); glPopMatrix();

    // number on both faces, unlit for clarity
    glDisable(GL_LIGHTING);
    glColor3f(1,1,1);
    std::ostringstream ss; ss << number;
    glPushMatrix();
    glTranslatef(-0.09f,2.64f,0.075f);
    glScalef(0.0014f,0.0014f,0.0014f);
    for (size_t i=0;i<ss.str().size();++i) glutStrokeCharacter(GLUT_STROKE_ROMAN, ss.str()[i]);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}


void drawTerminalBrandSign() {
    glPushMatrix();
    // Dark sign board on the front fascia
    setMaterial(0.06f,0.08f,0.10f,70,0.82f);
    glPushMatrix();
    glTranslatef(0.0f,0.0f,0.02f);
    cube(7.40f,0.70f,0.08f);
    glPopMatrix();

    // Brand text on sign board
    glDisable(GL_LIGHTING);
    glColor3f(0.90f, 0.97f, 1.00f);
    glPushMatrix();
    glTranslatef(-3.15f,-0.14f,0.07f);
    glScalef(0.0026f,0.0026f,0.0026f);
    for (size_t i = 0; i < TERMINAL_BRAND.size(); ++i) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, TERMINAL_BRAND[i]);
    }
    glPopMatrix();
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void drawShelter(float length = 10.0f, int route = 1) {
    glPushMatrix();

    // Posts.
    setMaterial(0.08f,0.085f,0.10f,65,0.90f);
    for (float x=-length/2+0.7f; x<=length/2-0.5f; x+=2.3f) {
        glPushMatrix(); glTranslatef(x,1.75f,-1.55f); cube(0.14f,3.5f,0.14f); glPopMatrix();
        glPushMatrix(); glTranslatef(x,1.75f, 1.55f); cube(0.14f,3.5f,0.14f); glPopMatrix();
    }

    // Roof frame + metal texture.
    setMaterial(0.15f,0.16f,0.18f,65,0.90f);
    glPushMatrix(); glTranslatef(0,3.48f,0); texturedBox(length+0.55f,0.22f,3.75f,texMetal,8,3); glPopMatrix();

    // Slight raised center roof gives more complex silhouette.
    glPushMatrix();
    glTranslatef(0,3.68f,0);
    glRotatef(4.0f,0,0,1);
    texturedBox(length+0.10f,0.12f,1.65f,texMetal,8,2);
    glPopMatrix();

    // Benches.
    glPushMatrix(); glTranslatef(0,0,0.25f); drawBench(length-2.2f); glPopMatrix();

    // Route sign near one end.
    glPushMatrix(); glTranslatef(-length/2+0.75f,0,-2.15f); drawRouteSign(route); glPopMatrix();

    glPopMatrix();
}

// ============================================================================
// Scene: terminal building, interior, windows and picture frame
// ============================================================================

void drawTransparentGlass(float w, float h) {
    // V27 polished glass: softer day tone + stronger layered reflections.
    // V69: base tint lightened and its opacity roughly halved - the previous
    // (0.42,0.57,0.66) slate-blue at up to 0.22 alpha read as a big heavy
    // blue-tinted panel rather than clear glass, especially from camera
    // preset 3 where the window fills most of the view.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    float glassAlpha = mixf(0.05f, 0.15f, dayNightBlend);
    setPBRMaterial(0.68f,0.80f,0.86f,0.05f,0.04f, glassAlpha);
    glBegin(GL_QUADS);
        glNormal3f(0,0,-1);
        glVertex3f(-w/2,0,0);
        glVertex3f( w/2,0,0);
        glVertex3f( w/2,h,0);
        glVertex3f(-w/2,h,0);
    glEnd();

    glDisable(GL_LIGHTING);
    float sweep = 0.5f + 0.5f * std::sin(ledPulse * 0.55f);
    float bandX = mixf(-w*0.22f, w*0.22f, sweep);

    // Large cool sky reflection. V69: alpha roughly halved so it reads as a
    // faint reflection instead of doubling up with the base tint above.
    glColor4f(mixf(0.86f,0.64f,dayNightBlend), mixf(0.94f,0.75f,dayNightBlend), 1.0f, mixf(0.07f,0.06f,dayNightBlend));
    glBegin(GL_QUADS);
        glVertex3f(-w*0.48f, h*0.58f, -0.006f);
        glVertex3f( w*0.48f, h*0.58f, -0.006f);
        glVertex3f( w*0.48f, h*0.92f, -0.006f);
        glVertex3f(-w*0.48f, h*0.92f, -0.006f);
    glEnd();

    // Moving diagonal highlight.
    glColor4f(0.95f,0.99f,1.0f, mixf(0.12f,0.10f,dayNightBlend));
    glBegin(GL_QUADS);
        glVertex3f(bandX - w*0.20f, h*0.06f, -0.007f);
        glVertex3f(bandX - w*0.05f, h*0.06f, -0.007f);
        glVertex3f(bandX + w*0.18f, h*0.94f, -0.007f);
        glVertex3f(bandX + w*0.03f, h*0.94f, -0.007f);
    glEnd();

    // Warm terminal interior reflection.
    glColor4f(1.0f,0.82f,0.56f, mixf(0.05f,0.16f,dayNightBlend));
    glBegin(GL_QUADS);
        glVertex3f(-w*0.46f, h*0.18f, -0.005f);
        glVertex3f( w*0.46f, h*0.18f, -0.005f);
        glVertex3f( w*0.46f, h*0.34f, -0.005f);
        glVertex3f(-w*0.46f, h*0.34f, -0.005f);
    glEnd();

    // Edge glints.
    glColor4f(0.96f,0.99f,1.0f, mixf(0.08f,0.10f,dayNightBlend));
    glBegin(GL_QUADS);
        glVertex3f(-w*0.44f, h*0.04f, -0.008f);
        glVertex3f(-w*0.40f, h*0.04f, -0.008f);
        glVertex3f(-w*0.32f, h*0.96f, -0.008f);
        glVertex3f(-w*0.36f, h*0.96f, -0.008f);

        glVertex3f( w*0.30f, h*0.08f, -0.008f);
        glVertex3f( w*0.34f, h*0.08f, -0.008f);
        glVertex3f( w*0.42f, h*0.92f, -0.008f);
        glVertex3f( w*0.38f, h*0.92f, -0.008f);
    glEnd();

    glEnable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawWindowUnit(float w, float h) {
    // Slim charcoal aluminium frame around each glass panel.
    const float frame = 0.035f;
    setMaterial(0.10f,0.11f,0.12f,85,0.88f);

    glPushMatrix(); glTranslatef(-w/2, h/2, 0.018f); cube(frame, h + frame, 0.055f); glPopMatrix();
    glPushMatrix(); glTranslatef( w/2, h/2, 0.018f); cube(frame, h + frame, 0.055f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, frame/2, 0.018f); cube(w + frame, frame, 0.055f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, h-frame/2, 0.018f); cube(w + frame, frame, 0.055f); glPopMatrix();

    // Horizontal transom, common in real terminal curtain-wall windows.
    glPushMatrix(); glTranslatef(0, h*0.66f, 0.018f); cube(w, 0.030f, 0.050f); glPopMatrix();

    drawTransparentGlass(w-frame*1.6f, h-frame*1.6f);
}

void drawPictureFrame() {
    // A wall-mounted route map / picture frame.
    setMaterial(0.35f,0.16f,0.045f,34,0.45f);
    glPushMatrix(); glTranslatef(0,0,0); cube(3.10f,1.85f,0.12f); glPopMatrix();
    setMaterial(0.82f,0.86f,0.88f,18,0.20f);
    glPushMatrix(); glTranslatef(0,0,-0.075f); cube(2.72f,1.47f,0.05f); glPopMatrix();

    // Simple colored route lines.
    glDisable(GL_LIGHTING);
    glLineWidth(5.0f);
    glBegin(GL_LINE_STRIP);
        glColor3f(0.05f,0.45f,0.80f);
        glVertex3f(-1.05f,-0.40f,-0.11f); glVertex3f(-0.30f,0.30f,-0.11f); glVertex3f(0.55f,-0.12f,-0.11f); glVertex3f(1.05f,0.44f,-0.11f);
    glEnd();
    glBegin(GL_LINE_STRIP);
        glColor3f(0.08f,0.65f,0.20f);
        glVertex3f(-1.10f,0.42f,-0.115f); glVertex3f(-0.55f,-0.12f,-0.115f); glVertex3f(0.15f,0.35f,-0.115f); glVertex3f(0.92f,-0.38f,-0.115f);
    glEnd();
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

void drawChairRow() {
    // V45: cleaner waiting lounge seating layout.
    for (int i=0;i<3;++i) {
        glPushMatrix();
        glTranslatef((i-1)*1.15f,0,0);
        setMaterial(0.08f,0.30f,0.52f,45,0.55f);
        glPushMatrix(); glTranslatef(0,0.62f,0); cube(0.78f,0.12f,0.62f); glPopMatrix();
        glPushMatrix(); glTranslatef(0,0.98f,0.26f); cube(0.78f,0.70f,0.10f); glPopMatrix();
        setMaterial(0.05f,0.05f,0.06f,50,0.70f);
        glPushMatrix(); glTranslatef(-0.28f,0.32f,0); cube(0.06f,0.64f,0.06f); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.28f,0.32f,0); cube(0.06f,0.64f,0.06f); glPopMatrix();
        glPopMatrix();
    }
}


void drawCeilingLight(float x, float z) {
    // V45 warm LED ceiling panel for waiting room realism.
    glPushMatrix();
    glTranslatef(x,3.92f,z);
    setMaterial(0.92f,0.88f,0.70f,90,0.75f);
    glPushMatrix();
    glScalef(1.0f,0.04f,0.55f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // soft light glow panel
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,0.85f,0.55f,0.20f);
    glPushMatrix();
    glTranslatef(0,-0.08f,0);
    glScalef(1.4f,0.02f,0.8f);
    glutSolidCube(1.0f);
    glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawCeilingFan(float x, float z) {
    glPushMatrix();
    glTranslatef(x,3.80f,z);

    // Light brushed-aluminium downrod remains readable against the charcoal
    // ceiling in both day and night modes.
    setPBRMaterial(0.68f,0.72f,0.76f,0.72f,0.28f);
    glPushMatrix(); glRotatef(-90,1,0,0); cylinder(0.05f,0.55f,14); glPopMatrix();

    // Warm brass motor housing provides a clear centre point while spinning.
    glTranslatef(0,-0.10f,0);
    setPBRMaterial(0.88f,0.55f,0.13f,0.62f,0.30f);
    sphere(0.13f,14,10);

    glRotatef(fanAngle,0,1,0);
    setPBRMaterial(0.76f,0.80f,0.83f,0.58f,0.34f);
    for (int i=0;i<4;++i) {
        glPushMatrix();
        glRotatef(i*90.0f,0,1,0);
        glTranslatef(0.82f,0,0);
        cube(1.50f,0.07f,0.22f);
        glPopMatrix();
    }

    // A slim ivory underside is intentionally unlit: the room is viewed from
    // below, where normal ceiling lighting would otherwise make blades black.
    glDisable(GL_LIGHTING);
    glColor3f(mixf(0.92f,0.72f,dayNightBlend),
              mixf(0.90f,0.78f,dayNightBlend),
              mixf(0.82f,0.88f,dayNightBlend));
    for (int i=0;i<4;++i) {
        glPushMatrix();
        glRotatef(i*90.0f,0,1,0);
        glTranslatef(0.82f,-0.042f,0);
        cube(1.42f,0.012f,0.17f);
        glPopMatrix();
    }

    // Small amber centre highlight makes the rotating hub visible from the
    // interior camera without adding another large object to the room.
    glColor3f(mixf(1.00f,0.82f,dayNightBlend),
              mixf(0.68f,0.84f,dayNightBlend),
              mixf(0.16f,0.94f,dayNightBlend));
    glPushMatrix(); glTranslatef(0,-0.075f,0); sphere(0.085f,14,10); glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

// V49: externally visible continuous rotation for a clear viva demonstration.
// These compact rooftop ventilation fans are realistic terminal equipment and
// use the same animation state as the indoor ceiling fans.
void drawRotatingRoofVent(float x, float z) {
    glPushMatrix();
    glTranslatef(x,5.33f,z);

    // Fixed rooftop base and motor housing.
    setPBRMaterial(0.30f,0.32f,0.34f,0.82f,0.28f);
    glPushMatrix(); glRotatef(-90,1,0,0); cylinder(0.34f,0.26f,20); glPopMatrix();
    glPushMatrix(); glTranslatef(0,0.28f,0); sphere(0.19f,18,12); glPopMatrix();

    // Six metal blades rotate continuously around the world Y axis.
    glTranslatef(0,0.36f,0);
    glRotatef(fanAngle,0,1,0);
    setPBRMaterial(0.16f,0.18f,0.20f,0.90f,0.20f);
    for (int i=0;i<6;++i) {
        glPushMatrix();
        glRotatef(i*60.0f,0,1,0);
        glTranslatef(0.42f,0,0);
        glRotatef(18.0f,0,1,0);
        cube(0.62f,0.055f,0.18f);
        glPopMatrix();
    }
    setPBRMaterial(0.38f,0.40f,0.42f,0.88f,0.22f);
    sphere(0.14f,16,10);
    glPopMatrix();
}

void drawTerminalBuilding() {
    glPushMatrix();

    // Floor.
    glPushMatrix(); glTranslatef(0,0.08f,0); texturedBox(12.0f,0.16f,6.2f,texPavement,8,5); glPopMatrix();

    // Back wall.
    glPushMatrix(); glTranslatef(0,2.45f,3.0f); texturedBox(12.0f,4.9f,0.28f,texBrick,7,4); glPopMatrix();

    // Side walls.
    glPushMatrix(); glTranslatef(-5.85f,2.45f,0); texturedBox(0.30f,4.9f,6.0f,texBrick,4,4); glPopMatrix();
    glPushMatrix(); glTranslatef( 5.85f,2.45f,0); texturedBox(0.30f,4.9f,6.0f,texBrick,4,4); glPopMatrix();

    // Front facade: warm brick + slim dark aluminium mullions for a realistic terminal look.
    glPushMatrix(); glTranslatef(0,4.45f,-3.0f); texturedBox(12.0f,0.90f,0.28f,texBrick,8,1); glPopMatrix();
    glPushMatrix(); glTranslatef(0,0.45f,-3.0f); texturedBox(12.0f,0.90f,0.28f,texBrick,8,1); glPopMatrix();
    setMaterial(0.10f,0.11f,0.12f,80,0.86f);
    for (float x=-5.65f; x<=5.66f; x+=1.62f) {
        glPushMatrix(); glTranslatef(x,2.45f,-3.08f); cube(0.045f,3.20f,0.045f); glPopMatrix();
    }

    // Roof slab + modern fascia.
    setMaterial(0.12f,0.13f,0.15f,45,0.65f);
    glPushMatrix(); glTranslatef(0,5.05f,0); texturedBox(12.6f,0.35f,6.6f,texMetal,9,5); glPopMatrix();
    glPushMatrix(); glTranslatef(0,4.72f,-3.30f); cube(12.8f,0.55f,0.35f); glPopMatrix();

    // Unique terminal name on the front side.
    glPushMatrix(); glTranslatef(0.0f,4.72f,-3.49f); drawTerminalBrandSign(); glPopMatrix();

    // Entrance canopy for a more modern urban-station facade.
    setMaterial(0.16f,0.17f,0.19f,72,0.92f);
    glPushMatrix(); glTranslatef(0.0f,3.62f,-4.10f); texturedBox(4.80f,0.16f,1.75f,texMetal,4,2); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.95f,2.20f,-4.65f); cube(0.12f,2.55f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef( 1.95f,2.20f,-4.65f); cube(0.12f,2.55f,0.12f); glPopMatrix();

    // Realistic framed curtain-wall windows. From camera preset 3, the road,
    // buses and trees remain visible through the low-opacity glass.
    for (float x=-4.86f; x<=4.87f; x+=1.62f) {
        glPushMatrix();
        glTranslatef(x,0.88f,-3.18f);
        drawWindowUnit(1.40f,3.02f);
        drawV46GlassReflection();
        glPopMatrix();
    }

    // Automatic sliding glass door with dark aluminium frame.
    setMaterial(0.09f,0.10f,0.11f,90,0.92f);
    glPushMatrix(); glTranslatef(-0.78f,2.12f,-3.23f); cube(0.075f,2.62f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.78f,2.12f,-3.23f); cube(0.075f,2.62f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(0,3.43f,-3.23f); cube(1.62f,0.075f,0.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(0,0.82f,-3.23f); cube(1.62f,0.075f,0.10f); glPopMatrix();

    // Two transparent sliding door leaves.
    glPushMatrix(); glTranslatef(-0.39f,0.86f,-3.245f); drawTransparentGlass(0.72f,2.50f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.39f,0.86f,-3.245f); drawTransparentGlass(0.72f,2.50f); glPopMatrix();

    // Small door handles.
    setMaterial(0.55f,0.56f,0.58f,100,0.95f);
    glPushMatrix(); glTranslatef(-0.09f,2.02f,-3.31f); cube(0.035f,0.45f,0.045f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.09f,2.02f,-3.31f); cube(0.035f,0.45f,0.045f); glPopMatrix();

    // Front safety bollards at the pedestrian edge.
    setMaterial(0.16f,0.17f,0.18f,60,0.85f);
    for (float x=-3.1f; x<=3.1f; x+=1.55f) {
        glPushMatrix(); glTranslatef(x,0.45f,-4.95f); glRotatef(-90,1,0,0); cylinder(0.08f,0.90f,14); glPopMatrix();
    }

    // Ticket counters at the back.
    setMaterial(0.88f,0.88f,0.90f,35,0.45f);
    for (int i=0;i<3;++i) {
        glPushMatrix();
        glTranslatef(-3.2f + i*3.2f,1.05f,2.15f);
        cube(2.45f,1.10f,0.70f);
        setMaterial(0.07f,0.25f,0.45f,60,0.75f);
        glPushMatrix(); glTranslatef(0,0.68f,-0.36f); cube(1.75f,0.65f,0.05f); glPopMatrix();
        setMaterial(0.88f,0.88f,0.90f,35,0.45f);
        glPopMatrix();
    }

    // Chairs.
    glPushMatrix(); glTranslatef(-2.1f,0.0f,0.0f); drawChairRow(); glPopMatrix();
    glPushMatrix(); glTranslatef( 2.1f,0.0f,-1.2f); glRotatef(180,0,1,0); drawChairRow(); glPopMatrix();

    // Three stationary seated passengers make the waiting lounge feel used.
    // walking=false prevents the walk cycle; seated=true bends both knees.
    drawHuman(-3.25f,-0.04f,0.18f,0.42f,0.70f,false,0.0f,  0.0f,true);
    drawHuman(-0.95f,-0.04f,0.68f,0.24f,0.22f,false,0.0f,  0.0f,true);
    drawHuman( 2.10f,-1.16f,0.18f,0.55f,0.30f,false,0.0f,180.0f,true);

    // Picture/route map on back wall.
    glPushMatrix(); glTranslatef(3.3f,2.80f,2.82f); drawPictureFrame(); glPopMatrix();

    // One retained animated information board; older duplicate LED/text
    // systems were removed. This is local to the real terminal interior.
    drawAnimatedLEDDisplay();

    // Continuous rotating ceiling fans.
    drawCeilingFan(-2.4f,0.2f);
    drawCeilingFan( 2.4f,0.2f);

    // V45 ceiling LED lighting.
    drawCeilingLight(-3.0f,0.5f);
    drawCeilingLight(0.0f,0.5f);
    drawCeilingLight(3.0f,0.5f);

    // Visible from exterior camera shots; proves continuous rotation even when
    // the indoor ceiling fans are hidden by the roof.
    drawRotatingRoofVent(-2.10f,0.75f);
    drawRotatingRoofVent( 2.10f,0.75f);

    glPopMatrix();
}

// V47 clean interior used by camera preset 3.
// This room is intentionally minimal: one clear central aisle, two side seat
// rows, a framed glass wall, ceiling lights and the required rotating fan.
// Old counters, duplicate walls, passengers and decorative props are not drawn
// in this view, so nothing floats in front of the camera or blocks the window.
void drawCleanInteriorView() {
    // Warm neutral tiled floor.
    setPBRMaterial(0.58f,0.57f,0.54f,0.05f,0.72f);
    texturedQuadXZ(-7.2f,-4.25f,7.2f,4.10f,0.045f,texPavement,10,6);

    // Simple rear and side shell. The front remains a full-height glass wall.
    setPBRMaterial(0.70f,0.67f,0.61f,0.02f,0.82f);
    glPushMatrix(); glTranslatef(0.0f,2.15f,4.05f); cube(14.4f,4.30f,0.18f); glPopMatrix();
    glPushMatrix(); glTranslatef(-7.10f,2.15f,-0.05f); cube(0.20f,4.30f,8.20f); glPopMatrix();
    glPushMatrix(); glTranslatef( 7.10f,2.15f,-0.05f); cube(0.20f,4.30f,8.20f); glPopMatrix();

    // Clean charcoal ceiling instead of exposed overlapping roofs.
    setPBRMaterial(0.15f,0.16f,0.17f,0.35f,0.62f);
    glPushMatrix(); glTranslatef(0.0f,4.32f,-0.05f); texturedBox(14.5f,0.22f,8.35f,texMetal,9,5); glPopMatrix();

    // Full-width curtain-wall window. Each reflection is local to its own pane.
    for (float x=-6.30f; x<=6.31f; x+=1.80f) {
        glPushMatrix();
        glTranslatef(x,0.38f,-4.18f);
        drawWindowUnit(1.68f,3.48f);
        drawV46GlassReflection();
        glPopMatrix();
    }

    // Slim top/bottom frames keep the facade visually organized.
    setPBRMaterial(0.08f,0.09f,0.10f,0.82f,0.22f);
    glPushMatrix(); glTranslatef(0.0f,0.36f,-4.16f); cube(14.35f,0.12f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f,3.92f,-4.16f); cube(14.35f,0.12f,0.12f); glPopMatrix();

    // Only two neat side seating rows; the central walking path stays empty.
    glPushMatrix(); glTranslatef(-5.75f,0.0f,-0.15f); glRotatef(-90.0f,0,1,0); drawChairRow(); glPopMatrix();
    glPushMatrix(); glTranslatef( 5.75f,0.0f,-0.15f); glRotatef( 90.0f,0,1,0); drawChairRow(); glPopMatrix();

    // Seated passengers occupy selected chairs while preserving the aisle.
    drawHuman(-5.75f,-1.30f,0.22f,0.46f,0.72f,false,0.0f,-90.0f,true);
    drawHuman(-5.75f, 1.00f,0.72f,0.30f,0.22f,false,0.0f,-90.0f,true);
    drawHuman( 5.75f,-0.15f,0.20f,0.58f,0.34f,false,0.0f, 90.0f,true);

    // Essential ceiling fixtures only.
    drawCeilingFan(0.0f,-0.55f);
    drawCeilingLight(-3.25f,-0.55f);
    drawCeilingLight( 3.25f,-0.55f);
}

// ============================================================================
// Scene: windmill
// ============================================================================
void drawWindmill() {
    glPushMatrix();

    // Tower.
    setMaterial(0.50f,0.25f,0.08f,28,0.40f);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    cone(1.10f,5.7f,26);
    glPopMatrix();

    // Hub and rotating blades in the local XY plane.
    glTranslatef(0,5.7f,-0.18f);
    setMaterial(0.26f,0.13f,0.04f,42,0.65f);
    sphere(0.35f,20,14);

    glRotatef(windAngle,0,0,1);
    for (int i=0;i<4;++i) {
        glPushMatrix();
        glRotatef(i*90.0f,0,0,1);
        glTranslatef(0,1.75f,0);
        setMaterial(0.24f,0.11f,0.035f,38,0.50f);
        cube(0.34f,3.15f,0.18f);
        setMaterial(0.62f,0.64f,0.68f,62,0.90f);
        for (float y=-1.15f; y<=1.15f; y+=0.40f) {
            glPushMatrix(); glTranslatef(0,y,-0.12f); cube(0.62f,0.07f,0.06f); glPopMatrix();
        }
        glPopMatrix();
    }

    glPopMatrix();
}

void drawShrub(float scale = 1.0f) {
    glPushMatrix();
    glScalef(scale, scale, scale);
    setMaterial(0.20f,0.14f,0.08f,12,0.10f);
    glPushMatrix(); glTranslatef(0.0f,0.12f,0.0f); sphere(0.10f,10,8); glPopMatrix();

    const float c[][3] = {
        {0.13f,0.44f,0.14f}, {0.17f,0.50f,0.18f}, {0.11f,0.36f,0.12f}
    };
    const float p[][4] = {
        {-0.30f,0.26f, 0.02f,0.32f}, {0.00f,0.34f,0.18f,0.36f},
        { 0.28f,0.28f,-0.08f,0.30f}, {-0.08f,0.38f,-0.20f,0.28f},
        { 0.10f,0.22f, 0.00f,0.25f}
    };
    for (int i = 0; i < 5; ++i) {
        const float* col = c[i % 3];
        setMaterial(col[0], col[1], col[2], 18, 0.16f);
        glPushMatrix(); glTranslatef(p[i][0], p[i][1], p[i][2]); sphere(p[i][3],16,12); glPopMatrix();
    }
    glPopMatrix();
}

void drawDustbin(float x, float z) {
    glPushMatrix();
    glTranslatef(x,0,z);
    setMaterial(0.10f,0.20f,0.26f,55,0.82f);
    glPushMatrix(); glTranslatef(0,0.55f,0); cube(0.55f,0.85f,0.55f); glPopMatrix();
    setMaterial(0.18f,0.19f,0.21f,70,0.90f);
    glPushMatrix(); glTranslatef(0,1.02f,0); cube(0.60f,0.08f,0.60f); glPopMatrix();
    glPopMatrix();
}

void drawBillboard(float x, float z, float rotation = 0.0f) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rotation,0,1,0);
    setMaterial(0.16f,0.16f,0.17f,60,0.80f);
    glPushMatrix(); glTranslatef(-1.20f,2.35f,0); cube(0.12f,4.7f,0.12f); glPopMatrix();
    glPushMatrix(); glTranslatef( 1.20f,2.35f,0); cube(0.12f,4.7f,0.12f); glPopMatrix();
    setMaterial(0.92f,0.94f,0.96f,24,0.25f);
    glPushMatrix(); glTranslatef(0,4.20f,0); cube(3.55f,1.55f,0.12f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.10f,0.38f,0.67f); glPushMatrix(); glTranslatef(0,4.45f,0.07f); cube(2.80f,0.25f,0.02f); glPopMatrix();
    glColor3f(0.20f,0.70f,0.35f); glPushMatrix(); glTranslatef(0,4.02f,0.07f); cube(2.20f,0.18f,0.02f); glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}


struct CityBuildingSpec {
    float x, z;
    float w, h, d;
    float r, g, b;
    bool glassy;
};

// One shared layout drives both the static display-list meshes and the live
// haze/window overlays. Keeping transforms in one table guarantees alignment.
const CityBuildingSpec CITY_BUILDINGS[] = {
    {-35.0f,-18.0f, 7.2f, 9.0f,6.8f, 0.70f,0.68f,0.63f,false},
    {-34.0f, -2.0f, 7.0f,12.0f,6.0f, 0.66f,0.70f,0.76f,true },
    {-34.5f, 14.0f, 6.8f, 8.0f,5.5f, 0.76f,0.74f,0.70f,false},
    {-47.0f,  7.0f, 9.0f,10.0f,7.0f, 0.72f,0.72f,0.70f,false},
    { 35.0f,-17.0f, 7.2f,10.0f,6.0f, 0.70f,0.73f,0.77f,true },
    { 34.8f, -2.5f, 6.2f, 8.5f,5.6f, 0.80f,0.77f,0.73f,false},
    { 35.0f, 14.0f, 7.0f,12.5f,6.0f, 0.68f,0.71f,0.76f,true },
    { 47.0f,  8.0f, 9.2f,11.0f,7.0f, 0.74f,0.73f,0.70f,false},
    {-18.0f, 42.0f,10.5f, 7.2f,6.5f, 0.76f,0.74f,0.71f,false},
    {  3.0f, 42.0f,12.0f,12.5f,7.0f, 0.66f,0.71f,0.77f,true },
    { 24.0f, 42.0f,10.0f, 8.5f,6.0f, 0.80f,0.79f,0.75f,false},
    {-16.0f,-42.0f, 8.8f, 7.2f,5.4f, 0.73f,0.72f,0.69f,false},
    {  2.0f,-42.0f,10.2f, 6.8f,5.2f, 0.69f,0.72f,0.76f,true },
    { 19.0f,-42.0f, 8.8f, 7.5f,5.4f, 0.75f,0.73f,0.70f,false},
    {-42.0f,-55.0f,10.0f,22.0f,8.0f, 0.68f,0.69f,0.68f,false},
    {-20.0f,-58.0f, 8.5f,27.0f,7.2f, 0.63f,0.69f,0.75f,true },
    { 18.0f,-57.0f, 9.0f,24.0f,7.5f, 0.75f,0.72f,0.66f,false},
    { 42.0f,-54.0f,10.5f,20.0f,8.0f, 0.65f,0.70f,0.76f,true }
};
const int CITY_BUILDING_COUNT = sizeof(CITY_BUILDINGS) / sizeof(CITY_BUILDINGS[0]);

void drawCityBuilding(float w, float h, float d, float r, float g, float b, bool glassy = false) {
    glPushMatrix();

    // Soft building footprint shadow. (Already present pre-V67; alpha bumped
    // slightly so it reads as clearly as the vehicle/canopy shadows do.)
    drawSoftShadow(0.0f, 0.0f, w * 0.62f, d * 0.62f, 0.14f);

    if (glassy) {
        setMaterial(r,g,b,80,0.95f);
        glPushMatrix(); glTranslatef(0,h*0.5f,0); texturedBox(w,h,d,texMetal,6,8); glPopMatrix();
    } else {
        setMaterial(r,g,b,35,0.35f);
        glPushMatrix(); glTranslatef(0,h*0.5f,0); cube(w,h,d); glPopMatrix();
    }

    setMaterial(0.14f,0.15f,0.17f,65,0.85f);
    glPushMatrix(); glTranslatef(0,h+0.12f,0); cube(w+0.18f,0.18f,d+0.18f); glPopMatrix();

    for (float y=1.15f; y<h-0.65f; y+=1.00f) {
        setMaterial(0.14f,0.18f,0.22f,95,1.0f);
        for (float x=-w/2+0.55f; x<=w/2-0.55f; x+=0.82f) {
            glPushMatrix(); glTranslatef(x,y,d/2+0.03f); cube(0.42f,0.48f,0.03f); glPopMatrix();
            glPushMatrix(); glTranslatef(x,y,-d/2-0.03f); cube(0.42f,0.48f,0.03f); glPopMatrix();
        }
        for (float z=-d/2+0.55f; z<=d/2-0.55f; z+=0.82f) {
            glPushMatrix(); glTranslatef(w/2+0.03f,y,z); cube(0.03f,0.48f,0.42f); glPopMatrix();
            glPushMatrix(); glTranslatef(-w/2-0.03f,y,z); cube(0.03f,0.48f,0.42f); glPopMatrix();
        }
    }

    // Dhaka apartment/commercial character: balcony bands and sun-shading fins.
    if (!glassy) {
        setMaterial(0.62f,0.61f,0.58f,22,0.20f);
        for (float y=2.8f; y<h-0.8f; y+=3.0f) {
            glPushMatrix(); glTranslatef(0,y,d/2+0.24f); cube(w*0.88f,0.10f,0.46f); glPopMatrix();
        }
    } else {
        setMaterial(0.18f,0.20f,0.22f,70,0.78f);
        for (float x=-w*0.36f; x<=w*0.36f; x+=w*0.24f) {
            glPushMatrix(); glTranslatef(x,h*0.50f,d/2+0.08f); cube(0.10f,h*0.92f,0.12f); glPopMatrix();
        }
    }

    // Rooftop service room and black water tank are familiar Dhaka silhouettes.
    setMaterial(0.55f,0.54f,0.50f,20,0.22f);
    glPushMatrix(); glTranslatef(-w*0.22f,h+0.52f,0); cube(w*0.28f,0.82f,d*0.36f); glPopMatrix();
    setMaterial(0.08f,0.09f,0.10f,35,0.30f);
    glPushMatrix(); glTranslatef(w*0.23f,h+0.22f,0); glRotatef(-90,1,0,0); cylinder(0.38f,0.72f,16); glPopMatrix();
    glPopMatrix();
}

void drawCityBlocks() {
    for (int i = 0; i < CITY_BUILDING_COUNT; ++i) {
        const CityBuildingSpec &s = CITY_BUILDINGS[i];
        glPushMatrix();
        glTranslatef(s.x,0.0f,s.z);
        drawCityBuilding(s.w,s.h,s.d,s.r,s.g,s.b,s.glassy);
        glPopMatrix();
    }
}

void drawCityDynamicOverlays() {
    // Fog-free aerial perspective: alpha blending performs the colour mix
    // finalColor = base*(1-haze) + skyHorizon*haze on BUILDINGS ONLY. V67:
    // a real GL_FOG pass was deliberately NOT re-added here - fog in fixed-
    // function OpenGL tints every fragment regardless of whether lighting is
    // enabled, which is exactly what caused the original blue-tint bug on
    // the unlit road/ground (see the V62-V64 notes at the top of this file).
    // This hand-rolled per-building haze gives the same distant-depth cue
    // without ever touching the road, so it can be safely strengthened.
    const float golden = goldenFactor();
    float skyHazeR = mixf(0.85f,0.07f,dayNightBlend);
    float skyHazeG = mixf(0.90f,0.09f,dayNightBlend);
    float skyHazeB = mixf(0.62f,0.16f,dayNightBlend);
    skyHazeR = mixf(skyHazeR, 1.00f, golden*0.5f);
    skyHazeG = mixf(skyHazeG, 0.62f, golden*0.5f);
    skyHazeB = mixf(skyHazeB, 0.38f, golden*0.5f);
    const float night = clampf((dayNightBlend - 0.20f) / 0.80f,0.0f,1.0f);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    for (int i = 0; i < CITY_BUILDING_COUNT; ++i) {
        const CityBuildingSpec &s = CITY_BUILDINGS[i];
        const float dx = s.x - camX;
        const float dz = s.z - camZ;
        const float distance = std::sqrt(dx*dx + dz*dz);
        // V67: haze now starts a little closer (24 vs 28) and can reach a
        // stronger maximum (0.42 vs 0.34) for a more noticeable depth cue on
        // the farthest buildings, while still never touching the road/ground.
        const float haze = clampf((distance - 24.0f) / 110.0f,0.0f,0.42f);

        if (haze > 0.001f) {
            glColor4f(skyHazeR,skyHazeG,skyHazeB,haze);
            glPushMatrix();
            glTranslatef(s.x,s.h*0.50f,s.z);
            cube(s.w+0.10f,s.h+0.08f,s.d+0.10f);
            glPopMatrix();
        }

        // Warm lit windows remain live outside the display list. A stable
        // index pattern lights only selected panes, avoiding a flat grid.
        if (night > 0.001f) {
            glColor4f(1.0f,0.78f,0.38f,0.22f*night*(1.0f-haze*0.55f));
            glPushMatrix();
            glTranslatef(s.x,0.0f,s.z);
            int pane = 0;
            for (float y=1.15f; y<s.h-0.65f; y+=1.00f) {
                for (float x=-s.w/2+0.55f; x<=s.w/2-0.55f; x+=0.82f,++pane) {
                    if ((pane+i)%3 != 0) continue;
                    glPushMatrix(); glTranslatef(x,y, s.d/2+0.061f); cube(0.32f,0.36f,0.018f); glPopMatrix();
                    glPushMatrix(); glTranslatef(x,y,-s.d/2-0.061f); cube(0.32f,0.36f,0.018f); glPopMatrix();
                }
                for (float z=-s.d/2+0.55f; z<=s.d/2-0.55f; z+=0.82f,++pane) {
                    if ((pane+i)%4 != 0) continue;
                    glPushMatrix(); glTranslatef( s.w/2+0.061f,y,z); cube(0.018f,0.36f,0.32f); glPopMatrix();
                    glPushMatrix(); glTranslatef(-s.w/2-0.061f,y,z); cube(0.018f,0.36f,0.32f); glPopMatrix();
                }
            }
            glPopMatrix();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawStaticUrbanProps() {
    // Static roadside furniture. Traffic signals are intentionally excluded:
    // drawAnimatedTrafficLights() owns the complete poles/housings/lamps, so
    // the old duplicate signal geometry can no longer overlap it.
    drawBillboard(-9.0f,-29.0f,0.0f);
    drawBillboard( 23.5f,  0.0f,90.0f);

    // Bins and shrubs around pedestrian areas.
    drawDustbin(-10.2f,-11.0f);
    drawDustbin( 10.2f,-11.0f);
    glPushMatrix(); glTranslatef(-15.5f,0,-25.0f); drawShrub(1.2f); glPopMatrix();
    glPushMatrix(); glTranslatef( 15.0f,0,-25.2f); drawShrub(1.2f); glPopMatrix();
    glPushMatrix(); glTranslatef(-27.0f,0, 24.0f); drawShrub(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 27.0f,0, 24.0f); drawShrub(1.0f); glPopMatrix();
}


// ============================================================================
// V14 Human + Traffic Realism Upgrade
// ============================================================================

void drawHuman(float x, float z, float shirtR, float shirtG, float shirtB,
               bool walking, float walkPhase, float facingDeg, bool seated){
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(facingDeg,0,1,0);

    // Walking passengers use opposite arm/leg swings plus a very small body
    // bounce. Stationary people pass walking=false and remain perfectly still.
    const float swing = (walking && !seated) ? 23.0f * std::sin(walkPhase * 3.2f) : 0.0f;
    const float bob = (walking && !seated) ? 0.025f * std::fabs(std::sin(walkPhase * 3.2f)) : 0.0f;
    glTranslatef(0,bob,0);

    // Standing legs pivot from the hips. A seated passenger instead gets a
    // horizontal thigh plus a vertical lower leg, so the body truly sits on
    // the chair rather than merely being a shortened standing figure.
    setMaterial(0.05f,0.06f,0.08f,30,0.2f);
    if (seated) {
        for (float lx : {-0.09f,0.09f}) {
            glPushMatrix(); glTranslatef(lx,0.72f,-0.02f); glRotatef(180,0,1,0); cylinder(0.045f,0.34f,10); glPopMatrix();
            glPushMatrix(); glTranslatef(lx,0.72f,-0.34f); glRotatef(90,1,0,0); cylinder(0.045f,0.46f,10); glPopMatrix();
        }
    } else {
        glPushMatrix(); glTranslatef(-0.09f,0.92f,0); glRotatef(90.0f+swing,1,0,0); cylinder(0.045f,0.60f,10); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.09f,0.92f,0); glRotatef(90.0f-swing,1,0,0); cylinder(0.045f,0.60f,10); glPopMatrix();
    }

    // Torso extends upward.
    setMaterial(shirtR,shirtG,shirtB,45,0.35f);
    const float torsoBase = seated ? 0.68f : 0.88f;
    glPushMatrix(); glTranslatef(0,torsoBase,0); glRotatef(-90,1,0,0); cylinder(0.16f,0.65f,16); glPopMatrix();

    // Arms swing opposite to their matching legs.
    setMaterial(shirtR,shirtG,shirtB,45,0.35f);
    const float shoulderY = seated ? 1.23f : 1.43f;
    glPushMatrix(); glTranslatef( 0.20f,shoulderY,0); glRotatef(90.0f+swing*0.62f,1,0,0); cylinder(0.035f,0.40f,10); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.20f,shoulderY,0); glRotatef(90.0f-swing*0.62f,1,0,0); cylinder(0.035f,0.40f,10); glPopMatrix();

    // head
    setMaterial(0.72f,0.52f,0.38f,35,0.2f);
    glPushMatrix(); glTranslatef(0,seated ? 1.45f : 1.65f,0); sphere(0.13f,16,12); glPopMatrix();

    glPopMatrix();
}

void drawDriverCabin(){
    // Driver seat and steering wheel visible through windshield.
    setMaterial(0.12f,0.13f,0.15f,40,0.4f);
    glPushMatrix(); glTranslatef(2.15f,1.05f,0.45f); cube(0.35f,0.55f,0.35f); glPopMatrix();

    setMaterial(0.04f,0.04f,0.05f,80,0.8f);
    glPushMatrix();
    glTranslatef(2.25f,1.55f,0.45f);
    glRotatef(90,1,0,0);
    torus(0.025f,0.16f,12,24);
    glPopMatrix();
}

void drawAnimatedTrafficLights(){
    // State order is the real sequence: 0 Red -> 1 Green -> 2 Yellow -> Red.
    const float redLamp    = trafficLightState==0 ? 1.0f : 0.10f;
    const float greenLamp  = trafficLightState==1 ? 1.0f : 0.10f;
    const float yellowLamp = trafficLightState==2 ? 1.0f : 0.10f;

    float poles[][2]={{-22,-20},{22,-20},{-22,20},{22,20}};
    for(auto &p:poles){
        glPushMatrix();
        glTranslatef(p[0],0,p[1]);
        setMaterial(0.05f,0.05f,0.06f,70,0.8f);
        glPushMatrix(); glRotatef(-90,1,0,0); cylinder(0.04f,3.75f,12); glPopMatrix();

        setMaterial(0.02f,0.02f,0.02f,30,0.2f);
        glPushMatrix(); glTranslatef(0,3.45f,0); cube(0.32f,0.85f,0.25f); glPopMatrix();

        glDisable(GL_LIGHTING);
        glColor3f(redLamp,0.02f,0.01f); glPushMatrix(); glTranslatef(0,3.70f,0.14f); sphere(0.07f,12,8); glPopMatrix();
        glColor3f(yellowLamp,yellowLamp*0.76f,0.01f); glPushMatrix(); glTranslatef(0,3.45f,0.14f); sphere(0.07f,12,8); glPopMatrix();
        glColor3f(0.01f,greenLamp,0.03f); glPushMatrix(); glTranslatef(0,3.20f,0.14f); sphere(0.07f,12,8); glPopMatrix();
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }
}

// V67: TRAFFIC OBEYS SIGNALS. The east-west moving vehicles (movingBusX,
// taxiMotion) pass right by the signal poles above at x = -22 and x = +22.
// This returns true while trafficLightState is Red (0) and the vehicle's x
// is in a small "stop zone" just before either pole line, so timer() can
// simply skip that vehicle's position update for the frame - a cheap way to
// get believable stop-on-red/go-on-green behaviour without a full per-vehicle
// state machine. Only Red holds traffic; Yellow and Green both flow through,
// matching the everyday complaint ("vehicles ignore the red light").
bool trafficHeldAtSignal(float x) {
    if (trafficLightState != 0) return false;
    const float stopMarginBehind = 1.4f; // braking starts this far before the line
    const float stopMarginAhead  = 0.3f; // small allowance so it doesn't hover exactly on the line
    if (x > -22.0f - stopMarginBehind && x < -22.0f + stopMarginAhead) return true;
    if (x >  22.0f - stopMarginBehind && x <  22.0f + stopMarginAhead) return true;
    return false;
}

void drawUtilityPole(float x, float z, float h) {
    glPushMatrix();
    glTranslatef(x,0,z);
    setMaterial(0.34f,0.28f,0.18f,8,0.06f);
    glPushMatrix(); glRotatef(-90,1,0,0); cylinder(0.10f,h,12); glPopMatrix();
    setMaterial(0.18f,0.18f,0.18f,25,0.20f);
    glPushMatrix(); glTranslatef(0,h-0.8f,0); cube(1.0f,0.08f,0.08f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.12f,0.12f,0.12f);
    glBegin(GL_LINES);
    glVertex3f(-0.46f,h-0.80f,0.00f); glVertex3f(-7.0f,h-0.95f,0.00f);
    glVertex3f( 0.46f,h-0.80f,0.00f); glVertex3f( 7.0f,h-0.95f,0.00f);
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawAutoRickshaw(float x, float z, float rot, bool moving) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rot,0,1,0);
    if (moving) drawSoftShadow(0.0f, 0.0f, 1.10f, 0.62f, 0.12f);

    // Wheels
    setPBRMaterial(0.08f,0.08f,0.09f,0.60f,0.14f);
    glPushMatrix(); glTranslatef(-0.46f,0.22f, 0.46f); glRotatef(90,1,0,0); glScalef(0.50f,0.50f,0.50f); drawWheel(); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.46f,0.22f,-0.46f); glRotatef(90,1,0,0); glScalef(0.50f,0.50f,0.50f); drawWheel(); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.54f,0.22f, 0.0f);  glRotatef(90,1,0,0); glScalef(0.50f,0.50f,0.50f); drawWheel(); glPopMatrix();

    // Body
    setPBRMaterial(0.10f,0.48f,0.18f,0.22f,0.22f);
    glPushMatrix(); glTranslatef(-0.05f,0.55f,0.0f); cube(1.10f,0.42f,1.08f); glPopMatrix();
    setPBRMaterial(0.96f,0.78f,0.08f,0.18f,0.18f);
    glPushMatrix(); glTranslatef(0.05f,1.00f,0.0f); cube(1.08f,0.12f,1.04f); glPopMatrix();
    setPBRMaterial(0.12f,0.14f,0.16f,0.25f,0.20f);
    glPushMatrix(); glTranslatef(0.38f,0.76f,0.0f); cube(0.46f,0.28f,0.88f); glPopMatrix();

    // V67: thin black/white checker accent along the hood edge - a common
    // real-world CNG auto-rickshaw decal touch.
    glDisable(GL_LIGHTING);
    for (int i=0;i<6;++i) {
        float shade = (i%2==0) ? 0.04f : 0.95f;
        glColor3f(shade,shade,shade);
        glPushMatrix(); glTranslatef(-0.42f + i*0.165f, 1.065f, 0.0f); cube(0.15f,0.02f,1.06f); glPopMatrix();
    }
    glEnable(GL_LIGHTING);

    glDisable(GL_LIGHTING);
    glColor4f(0.72f,0.88f,0.96f,0.45f);
    glBegin(GL_QUADS);
    glVertex3f(0.18f,0.64f,0.43f); glVertex3f(0.54f,0.64f,0.43f); glVertex3f(0.54f,0.88f,0.43f); glVertex3f(0.18f,0.88f,0.43f);
    glVertex3f(0.18f,0.64f,-0.43f); glVertex3f(0.54f,0.64f,-0.43f); glVertex3f(0.54f,0.88f,-0.43f); glVertex3f(0.18f,0.88f,-0.43f);
    glEnd();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}


void drawCycleRickshaw(float x, float z, float rot, bool moving) {
    glPushMatrix();
    glTranslatef(x,0,z);
    glRotatef(rot,0,1,0);
    if (moving) drawSoftShadow(0.0f, 0.0f, 1.55f, 0.65f, 0.12f);

    // Wheels
    setPBRMaterial(0.09f,0.09f,0.10f,0.60f,0.12f);
    glPushMatrix(); glTranslatef(-0.65f,0.32f, 0.52f); glRotatef(90,1,0,0); glScalef(0.62f,0.62f,0.62f); drawWheel(); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.65f,0.32f,-0.52f); glRotatef(90,1,0,0); glScalef(0.62f,0.62f,0.62f); drawWheel(); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.86f,0.30f,0.0f);  glRotatef(90,1,0,0); glScalef(0.56f,0.56f,0.56f); drawWheel(); glPopMatrix();

    // Rear carriage frame
    setPBRMaterial(0.82f,0.10f,0.10f,0.18f,0.18f);
    glPushMatrix(); glTranslatef(-0.18f,0.66f,0.0f); cube(1.16f,0.16f,1.20f); glPopMatrix();
    setPBRMaterial(0.10f,0.44f,0.78f,0.18f,0.18f);
    glPushMatrix(); glTranslatef(-0.32f,1.06f,0.0f); cube(0.82f,0.10f,1.04f); glPopMatrix();
    setPBRMaterial(0.95f,0.78f,0.14f,0.18f,0.12f);
    glPushMatrix(); glTranslatef(-0.68f,0.95f,0.0f); cube(0.12f,0.72f,1.02f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.04f,0.95f,0.0f); cube(0.12f,0.72f,1.02f); glPopMatrix();

    // Passenger seat and footrest.
    setPBRMaterial(0.14f,0.16f,0.18f,0.18f,0.16f);
    glPushMatrix(); glTranslatef(-0.36f,0.86f,0.0f); cube(0.54f,0.20f,0.84f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.30f,0.52f,0.0f); cube(0.46f,0.05f,0.54f); glPopMatrix();

    // Front cycle frame.
    setPBRMaterial(0.12f,0.12f,0.13f,0.30f,0.12f);
    glPushMatrix(); glTranslatef(0.36f,0.54f,0.0f); cube(0.80f,0.05f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.54f,0.78f,0.0f); cube(0.05f,0.52f,0.05f); glPopMatrix();
    glPushMatrix(); glTranslatef(1.02f,0.74f,0.0f); cube(0.10f,0.05f,0.45f); glPopMatrix();

    // Colorful hood
    setPBRMaterial(0.08f,0.62f,0.24f,0.18f,0.12f);
    glPushMatrix(); glTranslatef(-0.32f,1.32f,0.0f); cube(0.92f,0.08f,1.14f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor4f(0.96f,0.92f,0.52f,0.22f);
    glPushMatrix(); glTranslatef(-0.32f,1.18f,0.0f); cube(0.82f,0.18f,1.02f); glPopMatrix();
    glEnable(GL_LIGHTING);

    // V67: thin gold trim accent along the hood edge - the decorated look
    // real Dhaka cycle-rickshaws are known for.
    setPBRMaterial(0.85f,0.68f,0.18f,0.55f,0.25f);
    glPushMatrix(); glTranslatef(-0.32f,1.365f,0.0f); cube(0.94f,0.03f,1.16f); glPopMatrix();

    // Driver / passenger hints
    drawHuman(0.78f,0.0f,0.16f,0.30f,0.55f);
    drawHuman(-0.38f,0.0f,0.70f,0.26f,0.24f);
    glPopMatrix();
}

void drawDividerPlantDust() {
    // Center divider greenery and dust detail for a more local street feel.
    const float shrubPos[][2] = {
        {-14.0f,-26.2f},{-8.5f,-26.2f},{-3.0f,-26.2f},{2.5f,-26.2f},{8.0f,-26.2f},{13.5f,-26.2f},
        {-14.0f, 26.2f},{-8.5f, 26.2f},{-3.0f, 26.2f},{2.5f, 26.2f},{8.0f, 26.2f},{13.5f, 26.2f}
    };
    for (const auto &p : shrubPos) {
        glPushMatrix(); glTranslatef(p[0],0,p[1]); drawShrub(0.42f); glPopMatrix();
        drawGrassTuft(p[0]-0.45f,p[1]+0.26f,0.52f);
        drawGrassTuft(p[0]+0.42f,p[1]-0.22f,0.48f);
    }

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.74f,0.66f,0.46f,0.10f);
    glPushMatrix(); glTranslatef(0.0f,0.025f,-20.0f); cube(44.0f,0.02f,0.55f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f,0.025f,20.0f);  cube(44.0f,0.02f,0.55f); glPopMatrix();
    glColor4f(0.78f,0.70f,0.52f,0.08f);
    glPushMatrix(); glTranslatef(-22.0f,0.025f,0.0f); cube(0.55f,0.02f,42.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 22.0f,0.025f,0.0f); cube(0.55f,0.02f,42.0f); glPopMatrix();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawVarietyVehicles(){
    // A restrained static Dhaka transport sample for the parked-vehicle list.
    // Older versions placed seven models here and made the roads look crowded.
    glPushMatrix(); glTranslatef(-27,0,8); glRotatef(90,0,1,0); drawMiniBus(0.94f,0.78f,0.12f,false); glPopMatrix();
    glPushMatrix(); glTranslatef(-12.0f,0,31.0f); drawAutoRickshaw(0,0,0,false); glPopMatrix();
    glPushMatrix(); glTranslatef(-30,0,-14); glRotatef(90,0,1,0); drawMotorcycle(0.14f,0.14f,0.16f); glPopMatrix();
}

void drawParkedMiniBuses() {
    // Two non-animated minibuses; compiled once into the parked-vehicle list.
    glPushMatrix(); glTranslatef(-28.0f,0,-8.0f);  glRotatef(90,0,1,0);  drawMiniBus(0.05f,0.55f,0.85f,false); glPopMatrix();
    glPushMatrix(); glTranslatef( 28.0f,0,11.0f);  glRotatef(-90,0,1,0); drawMiniBus(0.90f,0.90f,0.92f,false); glPopMatrix();
}

// ============================================================================
// Scene: transformable monument (object coordinate transformation)
// ============================================================================
// ============================================================================
// Scene: transformable monument (object coordinate transformation)
// ============================================================================
void drawTransformableMonument() {
    glPushMatrix();
    glTranslatef(objectTX, objectTY, objectTZ);   // TRANSLATION
    glRotatef(objectRotY, 0,1,0);                // ROTATION
    glScalef(objectScale, objectScale, objectScale); // SCALING

    // Base location in central plaza.
    glTranslatef(4.9f,0.0f,-1.0f);

    setMaterial(0.23f,0.24f,0.27f,48,0.75f);
    glPushMatrix(); glTranslatef(0,0.35f,0); cube(2.1f,0.70f,2.1f); glPopMatrix();

    setMaterial(0.42f,0.46f,0.52f,88,0.95f);
    glPushMatrix(); glTranslatef(0,1.45f,0); glRotatef(-90,1,0,0); cylinder(0.20f,2.20f,24); glPopMatrix();

    setMaterial(0.78f,0.58f,0.12f,92,1.0f);
    glPushMatrix(); glTranslatef(0,2.60f,0); glRotatef(90,1,0,0); torus(0.12f,0.65f,18,30); glPopMatrix();
    glPushMatrix(); glTranslatef(0,2.60f,0); sphere(0.28f,22,14); glPopMatrix();

    glPopMatrix();
}

// ============================================================================
// Scene composition
// ============================================================================
void drawDecorativeEnvironment() {
    // V67: roadside/front-row/avenue trees used to be drawn right here, but
    // this whole function is compiled once into the static LIST_LANDSCAPE
    // display list at startup. A display list bakes in whatever value a
    // variable had at COMPILE time and never re-reads it afterward, so a
    // swaying rotation placed inside it would freeze forever at its startup
    // angle. Trees now live in drawSwayingTrees() below, called every frame
    // OUTSIDE this static list instead. Shrubs/grass/poles don't move, so
    // they stay here where they belong (compiled once, drawn cheaply).

    // Small landscaped shrubs on the terminal island.
    // (-9.8, 8.7) is reserved for the visible rotating eco-windmill.
    glPushMatrix(); glTranslatef( 8.7f,0,-8.7f); drawShrub(0.95f); glPopMatrix();
    glPushMatrix(); glTranslatef(-8.5f,0, 1.8f); drawShrub(0.88f); glPopMatrix();
    glPushMatrix(); glTranslatef( 8.1f,0, 1.6f); drawShrub(0.84f); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.6f,0, 9.2f); drawShrub(0.90f); glPopMatrix();

    // Extra shrub groups around outer landscaped pockets.
    glPushMatrix(); glTranslatef(-34.8f,0,-30.4f); drawShrub(1.10f); glPopMatrix();
    glPushMatrix(); glTranslatef( 34.8f,0,-30.2f); drawShrub(1.10f); glPopMatrix();
    glPushMatrix(); glTranslatef(-34.6f,0, 30.2f); drawShrub(1.02f); glPopMatrix();
    glPushMatrix(); glTranslatef( 34.7f,0, 30.0f); drawShrub(1.02f); glPopMatrix();

    // Freestanding grass clumps along plaza edges and island greens.
    const float grassPts[][3] = {
        {-12.4f,-9.4f,1.10f}, {-10.8f,-7.9f,0.90f}, {11.2f,-9.0f,1.00f}, {9.6f,-7.6f,0.84f},
        {-11.6f, 8.8f,0.96f}, { 9.8f, 9.4f,1.06f}, {-36.2f,-33.2f,0.92f}, {36.0f,-33.0f,0.88f},
        {-35.8f,33.0f,0.94f},  {35.7f,32.8f,0.90f}, {-28.8f,-47.0f,0.86f}, {28.4f,-47.2f,0.82f}
    };
    for (const auto &g : grassPts) drawGrassTuft(g[0], g[1], g[2]);

    // V51 clean mode: roadside stalls and human body-part models were removed.
    // Trees, landscaping, buildings and traffic already provide enough detail.

    drawUtilityPole(-31.8f,-18.0f,6.6f);
    drawUtilityPole(-31.8f, 10.0f,6.6f);
    drawUtilityPole( 31.8f,-10.0f,6.6f);
    drawUtilityPole( 31.8f, 18.0f,6.6f);
}

// V67: every tree placement that used to be inline in drawDecorativeEnvironment
// (see the comment there for why), now called once per frame with a small
// per-tree sway angle so the whole scene reads as gently breezy rather than
// frozen. swayTime reuses windAngle - the same clock already driving the
// windmill/ceiling fans/roof vents - so the 'R' key pauses tree sway with them
// too, with no extra state to manage. Each tree gets a different phase offset
// (derived from its loop index) so they don't all sway in lockstep.
void drawSwayingTrees() {
    const float swayTime = windAngle * PI / 180.0f;
    const float swayAmp = 2.2f;

    // Urban roadside trees with more organic variation.
    const float sideZ[] = {-22.0f,-12.5f,-1.8f,8.8f,20.5f};
    const float leftScale[]  = {0.82f,0.94f,0.78f,0.88f,0.80f};
    const float rightScale[] = {0.90f,0.82f,0.98f,0.84f,0.92f};
    for (int i=0;i<5;++i) {
        float swayL = swayAmp * std::sin(swayTime + i*1.7f);
        float swayR = swayAmp * std::sin(swayTime + i*1.7f + 3.1f);
        glPushMatrix(); glTranslatef(-41.0f,0,sideZ[i]); drawTree(leftScale[i], swayL); glPopMatrix();
        glPushMatrix(); glTranslatef( 41.0f,0,sideZ[i]+1.2f); drawTree(rightScale[i], swayR); glPopMatrix();
    }

    const float frontX[] = {-26.0f,-10.0f,10.0f,26.0f};
    const float frontScale[] = {0.76f,0.70f,0.74f,0.72f};
    for (int i=0;i<4;++i) {
        float sway = swayAmp * std::sin(swayTime + i*2.3f + 1.0f);
        glPushMatrix(); glTranslatef(frontX[i],0,-48.0f); drawTree(frontScale[i], sway); glPopMatrix();
    }

    // Small landscaped trees on the terminal island.
    glPushMatrix(); glTranslatef( 9.7f,0, 8.7f); drawTree(0.78f, swayAmp*std::sin(swayTime+0.4f)); glPopMatrix();
    glPushMatrix(); glTranslatef(-9.7f,0,-8.8f); drawTree(0.66f, swayAmp*std::sin(swayTime+2.6f)); glPopMatrix();

    // Mid-distance avenue trees strengthen the Dhaka street scale.
    const float avenueZ[] = {-26.0f,-13.0f,1.0f,15.0f,28.0f};
    for (int i=0;i<5;++i) {
        float swayL = swayAmp * std::sin(swayTime + i*1.9f + 0.7f);
        float swayR = swayAmp * std::sin(swayTime + i*1.9f + 4.0f);
        glPushMatrix(); glTranslatef(-30.5f,0,avenueZ[i]); drawTree(0.60f + 0.04f*(i%2), swayL); glPopMatrix();
        glPushMatrix(); glTranslatef( 30.5f,0,avenueZ[i]+2.0f); drawTree(0.62f + 0.03f*(i%3), swayR); glPopMatrix();
    }
}

void drawDhakaTerminalLandscaping() {
    // Controlled green buffers inspired by the reference: lawn beside the
    // terminal, low hedges at the facade and a clean pedestrian edge.
    texturedQuadXZ(-11.55f,5.15f,-7.15f,11.55f,0.035f,texGrass,3,5);
    texturedQuadXZ(  7.75f,5.15f,11.55f,11.55f,0.035f,texGrass,3,5);

    for (float z=5.9f; z<=10.9f; z+=1.25f) {
        glPushMatrix(); glTranslatef(-7.65f,0,z); drawShrub(0.55f); glPopMatrix();
        glPushMatrix(); glTranslatef( 8.25f,0,z); drawShrub(0.55f); glPopMatrix();
    }

    // A slim planted median separates terminal traffic from the foreground road.
    texturedQuadXZ(-9.2f,-11.85f,9.2f,-11.10f,0.055f,texGrass,10,1);
    for (float x=-8.3f; x<=8.3f; x+=2.05f) {
        glPushMatrix(); glTranslatef(x,0,-11.48f); drawShrub(0.42f); glPopMatrix();
    }
}

void drawStreetLights() {
    for (float z=-10.0f; z<=10.0f; z+=5.0f) {
        drawLampPost(-11.1f,z,0);
        drawLampPost( 11.1f,z,180);
    }
    for (float x=-8.0f; x<=8.0f; x+=4.0f) {
        drawLampPost(x,-11.0f,90);
    }

    // Extra city-road lights.
    for (float z=-14.0f; z<=14.0f; z+=7.0f) {
        drawLampPost(-22.9f,z,0);
        drawLampPost( 22.9f,z,180);
    }
    for (float x=-16.0f; x<=16.0f; x+=8.0f) {
        drawLampPost(x,20.8f,90);
    }
}

void drawBusBaysAndPlatforms() {
    // Long canopies similar to the first provided terminal reference.
    glPushMatrix(); glTranslatef(-2.0f,0,-5.8f); drawShelter(11.0f,1); glPopMatrix();
    glPushMatrix(); glTranslatef(-2.2f,0, 0.6f); drawShelter(11.5f,2); glPopMatrix();
    glPushMatrix(); glTranslatef(-7.9f,0, 2.5f); glRotatef(90,0,1,0); drawShelter(7.2f,3); glPopMatrix();

    // Bus bay yellow stop lines.
    glDisable(GL_LIGHTING);
    glColor3f(1.0f,0.82f,0.05f);
    for (int i=0;i<3;++i) {
        glPushMatrix(); glTranslatef(-5.0f + i*5.0f,0.05f,-10.8f); cube(3.8f,0.025f,0.12f); glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawParkedBuses() {
    // Non-animated buses are compiled once into a display list.
    glPushMatrix(); glTranslatef(-16.9f,0,-4.5f); glRotatef(90,0,1,0); drawBus(0.86f,0.12f,0.10f,false); glPopMatrix();
    glPushMatrix(); glTranslatef( 17.0f,0, 3.5f); glRotatef(-90,0,1,0); drawBus(0.74f,0.76f,0.80f,false); glPopMatrix();

    // Bus at front bay like the modern red reference.
    glPushMatrix(); glTranslatef(5.0f,0,-15.8f); drawBus(0.84f,0.12f,0.10f,false); glPopMatrix();

}

void drawParkedVehicleNightOverlays() {
    // This function is intentionally called every frame OUTSIDE the parked
    // vehicle display list. Consequently the current dayNightBlend controls
    // the lamps instead of the daytime value used while compiling the list.
    float night = clampf((dayNightBlend - 0.20f) / 0.80f,0.0f,1.0f);
    if (night <= 0.001f) return;

    glPushMatrix(); glTranslatef(-16.9f,0,-4.5f); glRotatef( 90,0,1,0); drawBusNightOverlay(); glPopMatrix();
    glPushMatrix(); glTranslatef( 17.0f,0, 3.5f); glRotatef(-90,0,1,0); drawBusNightOverlay(); glPopMatrix();
    glPushMatrix(); glTranslatef(  5.0f,0,-15.8f);                         drawBusNightOverlay(); glPopMatrix();

    glPushMatrix(); glTranslatef(-28.0f,0,-8.0f); glRotatef( 90,0,1,0); drawMiniBusNightOverlay(); glPopMatrix();
    glPushMatrix(); glTranslatef( 28.0f,0,11.0f); glRotatef(-90,0,1,0); drawMiniBusNightOverlay(); glPopMatrix();
    glPushMatrix(); glTranslatef(-27.0f,0, 8.0f); glRotatef( 90,0,1,0); drawMiniBusNightOverlay(); glPopMatrix();

    // Small parked local vehicles get only a restrained pool, not a large
    // beam, so the surrounding black road remains clearly visible.
    glPushMatrix(); glTranslatef(-12.0f,0,31.0f);
        drawLightPool(0.85f,0.0f,0.52f,1.0f,0.88f,0.55f,0.035f*night);
    glPopMatrix();
    glPushMatrix(); glTranslatef(-30.0f,0,-14.0f); glRotatef(90,0,1,0);
        drawLightPool(0.90f,0.0f,0.48f,1.0f,0.90f,0.60f,0.030f*night);
    glPopMatrix();
}

void drawMovingVehicles() {
    // Continuously moving bus along the front road.
    glPushMatrix();
    glTranslatef(movingBusX,0,-18.25f);
    drawBus(0.84f,0.12f,0.10f);
    glPopMatrix();

    // Right vertical road: two lane-centred moving minibuses.
    glPushMatrix();
    glTranslatef(25.8f,0,movingCarZ);
    glRotatef(-90,0,1,0);
    drawMiniBus(0.88f,0.87f,0.82f);
    glPopMatrix();

    // These retained systems use trafficFlowX, taxiMotion and motorcycleMotion.
    drawMovingTraffic();
    drawV26TrafficFlow();
    drawV26BayBusAnimation();
}

void drawSkyElements() {
    drawSkyBackdrop();
    glDisable(GL_LIGHTING);

    // V67: 3-STAGE DAY CYCLE - reused for both the lower golden-hour sun
    // position and its warm tint below.
    const float golden = goldenFactor();

    // Camera-relative basis keeps the Sun/Moon high in the upper-left open sky
    // in every fixed view, away from the central building cluster.
    float yaw = yawDeg * PI / 180.0f;
    float pitch = pitchDeg * PI / 180.0f;
    float forwardX = std::cos(pitch) * std::cos(yaw);
    float forwardY = std::sin(pitch);
    float forwardZ = std::cos(pitch) * std::sin(yaw);
    float rightX = -std::sin(yaw);
    float rightZ =  std::cos(yaw);
    float upX = -std::sin(pitch) * std::cos(yaw);
    float upY =  std::cos(pitch);
    float upZ = -std::sin(pitch) * std::sin(yaw);
    // V67: the sun/moon sits noticeably lower in the sky during the brief
    // golden-hour window (a real sunset sun is near the horizon, not high up).
    const float celestialHeight = 31.0f - 14.0f * golden;
    float celestialX = camX + forwardX*72.0f - rightX*28.0f + upX*celestialHeight;
    float celestialY = camY + forwardY*72.0f                  + upY*celestialHeight;
    float celestialZ = camZ + forwardZ*72.0f - rightZ*28.0f + upZ*celestialHeight;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Transparent halos must not write depth; otherwise the first large halo
    // hides the brighter inner layers and makes the Sun look unclear.
    glDepthMask(GL_FALSE);

    // V68: matched to the reference's flat-design sun - one cohesive, solid
    // orange disc with only a soft edge, not a photographic white-hot centre
    // fading through a bright yellow rim. Night moon colours (the second
    // mixf argument in each line) and the golden-hour tint logic are
    // unchanged - golden hour just deepens this same orange further.

    // Soft orange outer edge, gradually becoming a soft blue moon halo.
    glColor4f(mixf(mixf(0.98f,0.55f,dayNightBlend), 1.00f, golden*0.5f),
              mixf(mixf(0.72f,0.68f,dayNightBlend), 0.35f, golden*0.5f),
              mixf(mixf(0.38f,1.00f,dayNightBlend), 0.10f, golden*0.5f),
              mixf(0.14f,0.10f,dayNightBlend));
    glPushMatrix(); glTranslatef(celestialX,celestialY,celestialZ); sphere(mixf(4.6f,4.5f,dayNightBlend),32,22); glPopMatrix();

    // Same flat orange continues into the rim - no separate bright-yellow ring.
    glColor4f(mixf(mixf(0.98f,0.68f,dayNightBlend), 1.00f, golden*0.45f),
              mixf(mixf(0.70f,0.80f,dayNightBlend), 0.45f, golden*0.45f),
              mixf(mixf(0.36f,1.00f,dayNightBlend), 0.16f, golden*0.45f),
              mixf(0.40f,0.20f,dayNightBlend));
    glPushMatrix(); glTranslatef(celestialX,celestialY,celestialZ); sphere(mixf(3.2f,3.1f,dayNightBlend),32,22); glPopMatrix();

    glDisable(GL_BLEND);

    // Opaque core: solid flat orange by day, cool pearl-white at night.
    glColor3f(mixf(mixf(0.98f,0.88f,dayNightBlend), 1.00f, golden*0.35f),
              mixf(mixf(0.69f,0.93f,dayNightBlend), 0.58f, golden*0.35f),
              mixf(mixf(0.34f,1.00f,dayNightBlend), 0.28f, golden*0.35f));
    glPushMatrix(); glTranslatef(celestialX,celestialY,celestialZ); sphere(mixf(2.30f,2.15f,dayNightBlend),36,26); glPopMatrix();

    // Small centre: a touch brighter/more saturated orange for gentle
    // roundness shading (never white), turning into a crisp white moon
    // highlight at night.
    glColor3f(mixf(mixf(1.00f,0.98f,dayNightBlend), 1.00f, golden*0.20f),
              mixf(mixf(0.74f,0.98f,dayNightBlend), 0.80f, golden*0.20f),
              mixf(mixf(0.40f,1.00f,dayNightBlend), 0.55f, golden*0.20f));
    glPushMatrix(); glTranslatef(celestialX,celestialY,celestialZ); sphere(mixf(1.35f,1.35f,dayNightBlend),32,22); glPopMatrix();
    // The celestial body is a background element. Restoring depth writes here
    // lets every later building, tree and vehicle naturally appear in front.
    glDepthMask(GL_TRUE);
    // Animated camera-relative cloud banks. Unlike the old fixed -Z clouds,
    // these remain visible in all seven manual camera views.
    if (dayNightBlend < 0.72f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        float a1 = 0.82f * (1.0f - dayNightBlend * 0.75f);
        float a2 = 0.58f * (1.0f - dayNightBlend * 0.75f);
        auto cameraCloud = [&](float side, float height, float distance,
                               float scale, float alpha) {
            float drift = cloudDrift * 0.22f;
            drawSkyCloudCluster(
                camX + forwardX*distance + rightX*(side + drift) + upX*height,
                camY + forwardY*distance                         + upY*height,
                camZ + forwardZ*distance + rightZ*(side + drift) + upZ*height,
                scale, alpha);
        };

        // Large dark ceiling-like masses plus brighter broken clouds mirror
        // the dramatic overcast/open-cyan balance in the reference picture.
        cameraCloud(-37.0f, 31.0f, 88.0f, 3.10f, a1);
        cameraCloud(  2.0f, 36.0f, 92.0f, 2.55f, a1);
        cameraCloud( 39.0f, 28.0f, 86.0f, 2.80f, a1);
        cameraCloud(-18.0f, 18.0f, 80.0f, 1.45f, a2);
        cameraCloud( 25.0f, 17.0f, 78.0f, 1.25f, a2);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // V69: a small flock of simple flapping gull-wing silhouettes, drawn
    // camera-relative (same trick as the clouds above) so they stay visible
    // in every camera preset. Birds are a daytime/dusk sight, so they fade
    // out well before full night. Each bird gets its own phase offset so the
    // flock doesn't flap or fly in perfect unison.
    if (dayNightBlend < 0.75f) {
        glDisable(GL_LIGHTING);
        const float birdShade = mixf(0.16f, 0.08f, dayNightBlend);
        glColor3f(birdShade, birdShade, birdShade*1.05f);
        auto cameraBird = [&](float side, float height, float distance, float phaseOffset) {
            const float loop = std::fmod(birdDrift + phaseOffset*9.0f, 70.0f) - 35.0f;
            const float bob = 0.6f * std::sin(birdFlapPhase * PI/180.0f * 0.3f + phaseOffset);
            const float bx = camX + forwardX*distance + rightX*(side+loop) + upX*height;
            const float by = camY + forwardY*distance                      + upY*(height+bob);
            const float bz = camZ + forwardZ*distance + rightZ*(side+loop) + upZ*height;
            const float flap = 22.0f * std::sin(birdFlapPhase * PI/180.0f + phaseOffset*2.0f);
            glPushMatrix();
            glTranslatef(bx,by,bz);
            glScalef(0.9f,0.9f,0.9f);
            // Two flattened wings meeting at the body form a simple "gull
            // mark" silhouette; the flap angle animates them up and down.
            glPushMatrix(); glRotatef(20.0f+flap,0,0,1); glTranslatef(0.5f,0,0); glRotatef(-30.0f,0,1,0); cube(1.0f,0.03f,0.22f); glPopMatrix();
            glPushMatrix(); glRotatef(-(20.0f+flap),0,0,1); glTranslatef(-0.5f,0,0); glRotatef(30.0f,0,1,0); cube(1.0f,0.03f,0.22f); glPopMatrix();
            glPopMatrix();
        };
        cameraBird(-10.0f, 12.0f, 55.0f, 0.0f);
        cameraBird( -8.0f, 13.2f, 56.0f, 1.4f);
        cameraBird(-12.0f, 10.5f, 54.0f, 2.7f);
        cameraBird( 14.0f, 11.0f, 58.0f, 4.1f);
        glEnable(GL_LIGHTING);
    }

    // Very light stars at deeper night.
    if (dayNightBlend > 0.68f) {
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        glColor3f(0.88f, 0.90f, 1.0f);
        for (int i = 0; i < 24; ++i) {
            float x = -85.0f + (i * 7.2f);
            float y = 31.0f + (i % 5) * 4.0f;
            float z = -90.0f + (i % 3) * 3.0f;
            glVertex3f(x, y, z);
        }
        glEnd();
    }

    glEnable(GL_LIGHTING);
}

// Superseded V7/V9 full-terminal overlays were removed in V60. The retained
// drawTerminalBuilding() + drawBusBaysAndPlatforms() pair is the single source
// of terminal architecture, which prevents roofs and platforms overlapping.

// The V9 environment was also superseded; its billboard, roof, parking and
// lounge layers duplicated the final building and road system.

void drawRainRoadReflection(){
    // roadWetness rises only while rain is active and fades after rain stops.
    // At zero this function exits, leaving genuinely dry, matte asphalt.
    // The neutral film strength is deliberately independent of day/night so
    // wet asphalt never changes hue when the scene lighting changes.
    // V67: overall intensity raised (0.13->0.20 base, plus a stronger active-
    // rain pulse) for a more visible wet-road/puddle feel.
    float baseAlpha = roadWetness * 0.20f;
    if(weatherRain) baseAlpha += roadWetness * (0.09f + 0.03f*std::sin(rainReflectionPulse*0.7f));
    if(baseAlpha <= 0.001f) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Broad low-alpha wet film across the loop roads. Neutral grey preserves
    // black asphalt instead of creating the previous blue floor effect.
    glColor4f(0.16f,0.16f,0.15f,baseAlpha*0.34f);
    glPushMatrix(); glTranslatef(-17.0f,0.022f,0.0f); cube(9.2f,0.004f,39.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 17.0f,0.022f,0.0f); cube(9.2f,0.004f,39.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f,0.022f,-16.0f); cube(43.0f,0.004f,7.2f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f,0.022f, 16.0f); cube(43.0f,0.004f,7.2f); glPopMatrix();

    // Main front-road sheen is neutral, not sky-blue. Only actual rain adds a
    // restrained cool cast farther below.
    glColor4f(0.14f,0.14f,0.13f,baseAlpha);
    glPushMatrix(); glTranslatef(0.0f,0.024f,-18.0f); cube(42.0f,0.006f,2.6f); glPopMatrix();

    // Thin reflective lane sheens.
    glColor4f(0.28f,0.27f,0.25f, baseAlpha * 0.46f);
    glPushMatrix(); glTranslatef(-8.5f,0.025f,-18.0f); cube(9.0f,0.004f,0.16f); glPopMatrix();
    glPushMatrix(); glTranslatef( 8.5f,0.025f,-18.0f); cube(9.0f,0.004f,0.16f); glPopMatrix();

    // Central road subtle reflection.
    glColor4f(0.15f,0.15f,0.14f, baseAlpha*0.52f);
    glPushMatrix(); glTranslatef(0.0f,0.024f,0.0f); cube(2.6f,0.006f,38.0f); glPopMatrix();

    // Roadside specular streaks / light traces. V67: brighter and with a
    // stronger pulse so wet-night reflections actually catch the eye.
    float night = clampf((dayNightBlend - 0.20f) / 0.80f, 0.0f, 1.0f);
    if (night > 0.001f) {
        float pulse = 0.028f * (0.5f + 0.5f*std::sin(rainReflectionPulse));
        glColor4f(1.0f,0.82f,0.46f, 0.05f*night + pulse*night);
        for(int i=0;i<5;i++){
            glPushMatrix();
            glTranslatef(-8.0f + i*4.0f,0.025f,-17.2f + (i%2)*0.35f);
            cube(1.6f,0.005f,0.18f);
            glPopMatrix();
        }
    }

    if(weatherRain){
        glColor4f(0.22f,0.22f,0.22f, 0.11f + 0.035f*std::sin(rainReflectionPulse));
        for(int i=0;i<10;i++){
            glPushMatrix();
            glTranslatef(-20+i*4,0.025f,-18+i*2);
            cube(2.0f,0.008f,0.45f);
            glPopMatrix();
        }
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}




void drawStaticCategory(StaticListOffset category) {
    if (staticListBase != 0) {
        glCallList(staticListBase + category);
        return;
    }

    // Safe fallback for a driver that cannot allocate display lists.
    switch (category) {
        case LIST_GROUND_ROADS: drawGroundAndRoads(); break;
        case LIST_CITY_BLOCKS: drawCityBlocks(); break;
        case LIST_LANDSCAPE: drawDecorativeEnvironment(); break;
        case LIST_INFRASTRUCTURE:
            drawFence();
            drawBusBaysAndPlatforms();
            drawDhakaTerminalLandscaping();
            drawStaticUrbanProps();
            drawV37Branding();
            drawV37RouteSigns();
            break;
        case LIST_PARKED_VEHICLES:
            drawParkedBuses();
            drawParkedMiniBuses();
            drawVarietyVehicles();
            break;
        default: break;
    }
}

void buildStaticDisplayLists() {
    staticListBase = glGenLists(STATIC_LIST_COUNT);
    if (staticListBase == 0) return;

    // These lists contain only geometry whose transforms never change. Any
    // visual state driven by dayNightBlend/nightMode is a live overlay outside.
    glNewList(staticListBase + LIST_GROUND_ROADS, GL_COMPILE);
        drawGroundAndRoads();
    glEndList();

    glNewList(staticListBase + LIST_CITY_BLOCKS, GL_COMPILE);
        drawCityBlocks();
    glEndList();

    glNewList(staticListBase + LIST_LANDSCAPE, GL_COMPILE);
        drawDecorativeEnvironment();
    glEndList();

    glNewList(staticListBase + LIST_INFRASTRUCTURE, GL_COMPILE);
        drawFence();
        drawBusBaysAndPlatforms();
        drawDhakaTerminalLandscaping();
        drawStaticUrbanProps();
        drawV37Branding();
        drawV37RouteSigns();
    glEndList();

    glNewList(staticListBase + LIST_PARKED_VEHICLES, GL_COMPILE);
        drawParkedBuses();
        drawParkedMiniBuses();
        drawVarietyVehicles();
    glEndList();
}

void drawSceneObjects() {
    // Camera 3 has its own clean render pass. This prevents the legacy terminal
    // layers from occupying the same room and keeps the outdoor city visible.
    if (cameraPreset == 3) {
        drawSkyElements();
        drawStaticCategory(LIST_GROUND_ROADS);
        drawStaticCategory(LIST_CITY_BLOCKS);
        drawStaticCategory(LIST_LANDSCAPE);
        drawStaticCategory(LIST_PARKED_VEHICLES);
        drawCityDynamicOverlays();
        drawParkedVehicleNightOverlays();
        drawStreetLights();
        drawAnimatedTrafficLights();
        drawSwayingTrees(); // V67: trees animate every frame, outside the static list

        // Exterior activity visible beyond the window.
        drawV28WindowViewTraffic();
        drawV26WalkingPassengers();
        drawRainRoadReflection();

        // Draw the uncluttered room last so its glass and frames remain crisp.
        drawCleanInteriorView();
        drawV27RoadSurfaceGlow();
        drawV27CityLightingAccent();
        drawRainEffect();
        return;
    }

    // Render the complete sky first. All city geometry is then guaranteed to
    // appear in front of the Sun/Moon rather than the celestial body cutting
    // through a building facade.
    drawSkyElements();
    drawStaticCategory(LIST_GROUND_ROADS);
    drawStaticCategory(LIST_CITY_BLOCKS);
    drawStaticCategory(LIST_LANDSCAPE);
    drawStaticCategory(LIST_INFRASTRUCTURE);
    drawStaticCategory(LIST_PARKED_VEHICLES);
    drawCityDynamicOverlays();
    drawParkedVehicleNightOverlays();
    drawStreetLights();
    drawAnimatedTrafficLights();
    drawSwayingTrees(); // V67: trees animate every frame, outside the static list

    // Soft scene shadows for large structures.
    drawSoftShadow(0.0f, -1.6f, 13.0f, 5.2f, 0.10f);   // main front canopy shadow
    drawSoftShadow(2.6f, 8.1f, 7.8f, 4.8f, 0.14f);     // terminal building footprint
    drawSoftShadow(-2.0f, -5.8f, 6.0f, 2.6f, 0.10f);   // bus bays
    drawSoftShadow(-2.2f,  0.6f, 6.4f, 2.6f, 0.10f);
    drawSoftShadow(-7.9f,  2.5f, 4.2f, 2.3f, 0.10f);

    // Main terminal building at back of central island.
    glPushMatrix();
    glTranslatef(2.6f,0,8.1f);
    drawTerminalBuilding();
    glPopMatrix();

    // One visible continuous windmill, placed in its reserved landscaped bay.
    glPushMatrix();
    glTranslatef(-9.8f,0.0f,8.7f);
    glScalef(0.62f,0.62f,0.62f);
    drawWindmill();
    glPopMatrix();

    drawMovingVehicles();
    drawV26WalkingPassengers();
    drawTransformableMonument();
    drawRainRoadReflection();
    drawV27RoadSurfaceGlow();
    drawV27TerminalNightGlow();
    drawV27CityLightingAccent();
    drawRainEffect();
    drawAxes3D(4.0f);
}

// ============================================================================
// Lighting
// ============================================================================

void setupLights() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_LIGHT2);

    // V67: 3-STAGE DAY CYCLE - used below to warm LIGHT0 and gently dim the
    // ambient the same brief moment the sky/sun turn golden.
    const float golden = goldenFactor();

    // V65: daytime global ambient raised from ~0.21-0.23 to ~0.42-0.44 so
    // building facades and trees no longer sit in deep unlit shadow at noon.
    // Night-side values (second argument to each mixf) are unchanged. V67:
    // dimmed and warmed slightly during golden hour, like real dusk light.
    GLfloat globalAmb[] = {
        mixf(mixf(0.42f,0.040f,dayNightBlend), 0.30f, golden*0.5f),
        mixf(mixf(0.42f,0.046f,dayNightBlend), 0.18f, golden*0.5f),
        mixf(mixf(0.44f,0.074f,dayNightBlend), 0.16f, golden*0.5f),
        1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    // LIGHT 0: vibrant sunlight by day / cooler dim moonlight by night.
    // V65: daytime ambient raised to ~0.22 and diffuse to a warm-white
    // (1.0, 0.98, 0.92) so daylight actually looks sunny. Night-side values
    // (second argument to each mixf) and specular are unchanged. V67: the
    // diffuse shifts toward a warm orange during golden hour, like real
    // low-angle sunset light.
    GLfloat pos0[] = {PRIMARY_LIGHT_X,PRIMARY_LIGHT_Y,PRIMARY_LIGHT_Z,0.0f};
    GLfloat amb0[] = {mixf(0.22f,0.03f,dayNightBlend),mixf(0.22f,0.04f,dayNightBlend),mixf(0.22f,0.07f,dayNightBlend),1};
    GLfloat dif0[] = {mixf(mixf(1.00f,0.20f,dayNightBlend), 1.00f, golden*0.5f),
                      mixf(mixf(0.98f,0.25f,dayNightBlend), 0.55f, golden*0.5f),
                      mixf(mixf(0.92f,0.40f,dayNightBlend), 0.30f, golden*0.5f),1};
    GLfloat spe0[] = {mixf(0.82f,0.30f,dayNightBlend),mixf(0.84f,0.36f,dayNightBlend),mixf(0.78f,0.56f,dayNightBlend),1};
    glLightfv(GL_LIGHT0,GL_POSITION,pos0);
    glLightfv(GL_LIGHT0,GL_AMBIENT,amb0);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,dif0);
    glLightfv(GL_LIGHT0,GL_SPECULAR,spe0);

    // LIGHT 1: terminal warm light stronger at night.
    GLfloat pos1[] = {mixf(-10.0f,2.6f,dayNightBlend),mixf(9.5f,5.8f,dayNightBlend),mixf(-7.5f,3.5f,dayNightBlend),1.0f};
    GLfloat amb1[] = {mixf(0.06f,0.07f,dayNightBlend),mixf(0.05f,0.05f,dayNightBlend),mixf(0.04f,0.03f,dayNightBlend),1};
    GLfloat dif1[] = {mixf(0.44f,1.0f,dayNightBlend),mixf(0.43f,0.78f,dayNightBlend),mixf(0.40f,0.48f,dayNightBlend),1};
    GLfloat spe1[] = {mixf(0.48f,1.0f,dayNightBlend),mixf(0.46f,0.84f,dayNightBlend),mixf(0.42f,0.56f,dayNightBlend),1};
    glLightfv(GL_LIGHT1,GL_POSITION,pos1);
    glLightfv(GL_LIGHT1,GL_AMBIENT,amb1);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,dif1);
    glLightfv(GL_LIGHT1,GL_SPECULAR,spe1);
    glLightf(GL_LIGHT1,GL_CONSTANT_ATTENUATION,mixf(0.95f,0.54f,dayNightBlend));
    glLightf(GL_LIGHT1,GL_LINEAR_ATTENUATION,mixf(0.020f,0.026f,dayNightBlend));

    // LIGHT 2: directional daylight sky-fill; it becomes gently cool only at
    // night. V66: this was previously a POSITIONAL light (w=1.0) with linear
    // attenuation, so anything more than ~30-50 units away barely received
    // it - which is why distant building walls facing away from LIGHT0 still
    // looked dark/night-like even at noon. Making it directional (w=0.0)
    // removes the distance falloff entirely, so it now lights the whole city
    // evenly, like real skylight. Attenuation calls below are harmless no-ops
    // for a directional light but are left in case this is ever repositioned.
    GLfloat pos2[] = {mixf(16.0f,0.0f,dayNightBlend),mixf(11.0f,7.4f,dayNightBlend),mixf(10.0f,-10.5f,dayNightBlend),0.0f};
    GLfloat amb2[] = {mixf(0.07f,0.04f,dayNightBlend),mixf(0.07f,0.045f,dayNightBlend),mixf(0.065f,0.065f,dayNightBlend),1};
    GLfloat dif2[] = {mixf(0.42f,0.34f,dayNightBlend),mixf(0.42f,0.40f,dayNightBlend),mixf(0.40f,0.56f,dayNightBlend),1};
    GLfloat spe2[] = {mixf(0.30f,0.40f,dayNightBlend),mixf(0.30f,0.48f,dayNightBlend),mixf(0.28f,0.66f,dayNightBlend),1};
    glLightfv(GL_LIGHT2,GL_POSITION,pos2);
    glLightfv(GL_LIGHT2,GL_AMBIENT,amb2);
    glLightfv(GL_LIGHT2,GL_DIFFUSE,dif2);
    glLightfv(GL_LIGHT2,GL_SPECULAR,spe2);
    glLightf(GL_LIGHT2,GL_CONSTANT_ATTENUATION,mixf(1.0f,0.64f,dayNightBlend));
    glLightf(GL_LIGHT2,GL_LINEAR_ATTENUATION,mixf(0.020f,0.024f,dayNightBlend));

    // LIGHT 3 + LIGHT 4: real local point lights at the two prominent lamp
    // posts beside the main foreground approach. They affect nearby lit
    // objects but cannot recolour road/ground because those surfaces are unlit.
    // They are explicitly disabled in day mode to avoid consuming light slots.
    if (nightMode) {
        const float lampStrength = clampf(dayNightBlend,0.0f,1.0f);
        GLfloat lampAmbient[]  = {0.018f*lampStrength,0.014f*lampStrength,0.008f*lampStrength,1.0f};
        GLfloat lampDiffuse[]  = {0.92f*lampStrength,0.66f*lampStrength,0.34f*lampStrength,1.0f};
        GLfloat lampSpecular[] = {1.00f*lampStrength,0.78f*lampStrength,0.46f*lampStrength,1.0f};
        GLfloat lampPos3[] = {-4.0f,4.20f,-11.0f,1.0f};
        GLfloat lampPos4[] = { 4.0f,4.20f,-11.0f,1.0f};

        glEnable(GL_LIGHT3);
        glLightfv(GL_LIGHT3,GL_POSITION,lampPos3);
        glLightfv(GL_LIGHT3,GL_AMBIENT,lampAmbient);
        glLightfv(GL_LIGHT3,GL_DIFFUSE,lampDiffuse);
        glLightfv(GL_LIGHT3,GL_SPECULAR,lampSpecular);
        glLightf(GL_LIGHT3,GL_CONSTANT_ATTENUATION,1.0f);
        glLightf(GL_LIGHT3,GL_LINEAR_ATTENUATION,0.11f);
        glLightf(GL_LIGHT3,GL_QUADRATIC_ATTENUATION,0.028f);

        glEnable(GL_LIGHT4);
        glLightfv(GL_LIGHT4,GL_POSITION,lampPos4);
        glLightfv(GL_LIGHT4,GL_AMBIENT,lampAmbient);
        glLightfv(GL_LIGHT4,GL_DIFFUSE,lampDiffuse);
        glLightfv(GL_LIGHT4,GL_SPECULAR,lampSpecular);
        glLightf(GL_LIGHT4,GL_CONSTANT_ATTENUATION,1.0f);
        glLightf(GL_LIGHT4,GL_LINEAR_ATTENUATION,0.11f);
        glLightf(GL_LIGHT4,GL_QUADRATIC_ATTENUATION,0.028f);
    } else {
        glDisable(GL_LIGHT3);
        glDisable(GL_LIGHT4);
    }
}

// V62: the former global OpenGL fog and low ground-fog overlay were removed.
// On older fixed-function drivers both passes mixed the sky colour into the
// road, making nominally black asphalt appear blue. Depth is now communicated
// by the sky gradient and layered city buildings instead.

// ============================================================================
// Camera / viewing-coordinate transformation
// ============================================================================
void updateLookDirection(float &lookX, float &lookY, float &lookZ) {
    float yaw = yawDeg * PI / 180.0f;
    float pitch = pitchDeg * PI / 180.0f;
    lookX = camX + std::cos(pitch) * std::cos(yaw);
    lookY = camY + std::sin(pitch);
    lookZ = camZ + std::cos(pitch) * std::sin(yaw);
}

void pointCameraAt(float x, float y, float z) {
    float dx = x - camX;
    float dy = y - camY;
    float dz = z - camZ;
    float horiz = std::sqrt(dx*dx + dz*dz);
    yawDeg = std::atan2(dz, dx) * 180.0f / PI;
    pitchDeg = std::atan2(dy, horiz) * 180.0f / PI;
}


void setCameraPreset(int p) {
    // V45 INTERIOR REALISM POLISH
    // No automatic cinematography.
    // Each key gives a fixed professional presentation camera.
    cameraPreset = p;
    cinematicTour = false;

    switch(p) {
        case 1: // Full terminal composition
            camX = 46.0f; camY = 18.0f; camZ = 46.0f;
            yawDeg = -135.0f; pitchDeg = -20.0f;
            break;

        case 2: // Modern bus hero
            camX = 8.5f; camY = 3.2f; camZ = -18.0f;
            yawDeg = -150.0f; pitchDeg = -10.0f;
            break;

        case 3: // FIXED: Inside waiting room -> glass window -> outside traffic view
            // Camera stays inside the terminal lounge.
            // Looking outward through the front curtain-wall glass.
            // Shows chairs/interior + road + buses + trees outside.
            camX = 0.0f;
            camY = 1.55f;
            camZ = 0.75f;
            yawDeg = -90.0f;
            pitchDeg = -1.0f;
            break;

        case 4: // Road traffic
            camX = 28.0f; camY = 5.0f; camZ = 20.0f;
            yawDeg = -145.0f; pitchDeg = -14.0f;
            break;

        case 5: // Passenger bus bay
            camX = -14.0f; camY = 3.5f; camZ = -8.0f;
            yawDeg = -40.0f; pitchDeg = -8.0f;
            break;

        case 6: // Dhaka city environment - elevated wide establishing shot
            camX = -42.0f;
            camY = 17.0f;
            camZ = 38.0f;
            pointCameraAt(0.0f,3.5f,1.5f);
            break;

        case 7: // Terminal facade hero shot for the final viva screenshot
            camX = 18.0f;
            camY = 7.2f;
            camZ = -28.0f;
            pointCameraAt(2.6f,2.8f,6.0f);
            break;
    }
}

void moveCamera(float forward, float strafe) {
    // Camera preset 3 is a simplified demonstration room rather than a second
    // copy of the full terminal shell. Translation is intentionally locked in
    // this preset so free-camera movement cannot reveal a mismatched exterior;
    // looking around with arrows/mouse remains available. This is option (b)
    // from the consistency fix and is the smallest correct, explainable rule.
    if (cameraPreset == 3) return;

    float yaw = yawDeg * PI / 180.0f;
    float fx = std::cos(yaw), fz = std::sin(yaw);
    float sx = -fz, sz = fx;
    camX += forward * fx + strafe * sx;
    camZ += forward * fz + strafe * sz;
}

// ============================================================================
// Display / callbacks
// ============================================================================

// ============================================================================
// V37 FINAL SUBMISSION POLISH
// - Dhaka route identity and local branding
// - Final screenshot camera preset
// - Viva clean presentation mode
// - Requirement explanation support
// ============================================================================

void drawV37Branding(){
    // Main terminal identity board.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.1f);
    glDisable(GL_LIGHTING);
    glColor3f(0.05f,0.55f,0.22f);
    glPushMatrix();
    glTranslatef(-4.5f,5.6f,-16.0f);
    glScalef(0.003f,0.003f,0.003f);
    std::string t="DHAKA CITY TERMINAL";
    for(char c:t) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();

    glColor3f(1.0f,0.80f,0.15f);
    glPushMatrix();
    glTranslatef(-3.8f,5.05f,-16.0f);
    glScalef(0.0018f,0.0018f,0.0018f);
    std::string s="UTTARA - AIRPORT - MOTIJHEEL";
    for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawV37RouteSigns(){
    // Local Dhaka bus route boards.
    const float signs[][3]={{-30,0,-18},{30,0,-18},{-28,0,18}};
    const char* routes[]={"MIRPUR 10","GULISTAN","DHAKA NORTH"};
    for(int i=0;i<3;i++){
        glPushMatrix();
        glTranslatef(signs[i][0],1.8f,signs[i][2]);
        setMaterial(0.04f,0.20f,0.10f,35,0.35f);
        cube(2.8f,1.0f,0.10f);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f,0.85f,0.2f);
        glPushMatrix();
        glTranslatef(-1.15f,0,0.08f);
        glScalef(0.0015f,0.0015f,0.0015f);
        for(const char c:std::string(routes[i])) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
        glPopMatrix();
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }
}

void display() {
    glClearColor(mixf(0.30f,0.018f,dayNightBlend), mixf(0.48f,0.032f,dayNightBlend), mixf(0.58f,0.078f,dayNightBlend), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // The one retained cinematic system updates BEFORE gluLookAt(), avoiding
    // the one-frame camera lag caused by the old scene-end update.
    drawFinalCinematicCamera();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float lookX, lookY, lookZ;
    updateLookDirection(lookX, lookY, lookZ);
    gluLookAt(camX,camY,camZ, lookX,lookY,lookZ, 0,1,0);

    setupLights();
    // V33 optimized render order: stable scene pass for smoother demo FPS.
    drawSceneObjects();
    drawHUD();
    fpsCounter++;
    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    winW = w; winH = h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(55.0, (double)w/(double)h, 0.10, 160.0);
    glMatrixMode(GL_MODELVIEW);
}



// ============================================================================
// V15 FINAL PRESENTATION POLISH
// ============================================================================

void drawRainEffect(){
    if(!weatherRain) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.72f,0.73f,0.74f,0.42f);
    glBegin(GL_LINES);
    for(int i=0;i<180;i++){
        float x=-45.0f + (i*7)%90;
        float z=-45.0f + (i*11)%90;
        float y=12.0f - fmod(rainOffset + i*0.37f,12.0f);
        glVertex3f(x,y,z);
        glVertex3f(x+0.12f,y-1.0f,z+0.05f);
    }
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawAnimatedLEDDisplay(){
    // Local coordinates inside drawTerminalBuilding(): a single back-wall
    // information panel, safely above the ticket counters.
    setMaterial(0.025f,0.030f,0.035f,90,0.95f);
    glPushMatrix(); glTranslatef(-1.35f,3.72f,2.80f); cube(4.55f,0.72f,0.10f); glPopMatrix();

    glDisable(GL_LIGHTING);
    float glow=0.65f+0.35f*std::sin(ledPulse);
    glColor3f(1.0f,0.75f*glow,0.15f);
    for(int i=0;i<12;i++){
        glPushMatrix();
        glTranslatef(-3.05f+i*0.31f,3.72f,2.735f);
        cube(0.11f,0.17f,0.025f);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

void drawMovingTraffic(){
    // Rear city road: one minibus plus two distinctly Dhaka vehicles.
    glPushMatrix();
    glTranslatef(trafficFlowX,0,18.2f);
    drawMiniBus(0.34f,0.36f,0.40f);
    glPopMatrix();

    // Dhaka local service-lane traffic; deliberately sparse to avoid clutter.
    drawAutoRickshaw(-14.0f + std::fmod(trafficFlowX*1.5f + 16.0f, 32.0f), -25.0f, 0.0f, true);
    drawCycleRickshaw(-12.0f + std::fmod(trafficFlowX*0.75f + 12.0f, 24.0f), 24.8f, 0.0f, true);
}


void drawV26WalkingPassengers(){
    // Crosswalk pedestrians and plaza walkers.
    for(int i=0;i<4;i++){
        float x = -4.5f + i*2.2f;
        float z = -20.0f + std::fmod(crossingWalk + i*1.35f, 10.0f);
        drawHuman(x, z, 0.18f + 0.12f*(i%3), 0.28f + 0.10f*(i%2),
                  0.62f - 0.08f*(i%2), true, crossingWalk + i*1.35f, 0.0f);
    }
    for(int i=0;i<3;i++){
        float loop = std::fmod(crossingWalk*0.55f + i*2.2f, 11.0f);
        float x = -8.0f + loop*1.45f;
        float z = 9.4f + 0.35f*std::sin(loop*0.6f + i);
        drawHuman(x, z, 0.55f - 0.10f*i, 0.30f + 0.08f*i,
                  0.18f + 0.12f*i, true, loop + i*0.8f, 90.0f);
    }
}

void drawV26TrafficFlow(){
    // Secondary boulevard uses the two remaining motion states exactly once.
    glPushMatrix();
    glTranslatef(taxiMotion,0,-20.6f);
    drawMiniBus(0.95f,0.78f,0.12f);
    glPopMatrix();

    // Side road motorcycle.
    glPushMatrix();
    glTranslatef(30.0f,0,motorcycleMotion);
    glRotatef(-90,0,1,0);
    drawMotorcycle(0.76f,0.12f,0.08f);
    glPopMatrix();

}

void drawV26BayBusAnimation(){
    // Bus arrival -> short stop -> departure cycle in the terminal bay.
    float x = -28.0f;
    if (bayBusPhase < 12.0f) {
        x = -28.0f + bayBusPhase * 1.92f; // arrive
    } else if (bayBusPhase < 20.0f) {
        x = -5.0f; // stop at bay
    } else {
        x = -5.0f + (bayBusPhase - 20.0f) * 1.95f; // depart
    }

    glPushMatrix();
    glTranslatef(x,0,-13.9f);
    drawBus(0.84f,0.12f,0.10f);
    glPopMatrix();

    // Highlight active stop phase with open-door feel and waiting passengers.
    if (bayBusPhase >= 12.0f && bayBusPhase < 22.0f) {
        // The bus model's own door leaves animate via busDoorAngle; the old
        // detached world-space door geometry was removed to prevent overlap.
        for (int i=0;i<3;i++) {
            drawHuman(-7.4f + i*0.9f, -11.9f + i*0.15f,
                      0.15f + 0.12f*i, 0.36f, 0.62f - 0.10f*i,
                      true, bayBusPhase + i*0.9f, 90.0f);
        }
    }
}




void drawV46GlassReflection(){
    // V47 FIX: all coordinates are local to one window pane.
    // The previous facade-size overlay was repeated once per pane and created
    // the giant dark rectangles visible inside the room.
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glColor4f(0.55f,0.75f,0.92f,0.035f);
    glBegin(GL_QUADS);
        glVertex3f(-0.64f,0.08f,-0.040f);
        glVertex3f( 0.64f,0.08f,-0.040f);
        glVertex3f( 0.64f,2.82f,-0.040f);
        glVertex3f(-0.64f,2.82f,-0.040f);
    glEnd();

    // One narrow moving highlight, contained inside the pane.
    float sweep = 0.12f * std::sin(ledPulse * 0.55f);
    glColor4f(1.0f,0.97f,0.86f,0.075f);
    glBegin(GL_QUADS);
        glVertex3f(-0.48f+sweep,0.16f,-0.045f);
        glVertex3f(-0.34f+sweep,0.16f,-0.045f);
        glVertex3f( 0.34f+sweep,2.74f,-0.045f);
        glVertex3f( 0.20f+sweep,2.74f,-0.045f);
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void drawV28WindowViewTraffic(){
    // Dedicated moving vehicles visible from the inside-window camera preset.
    float frontCarX = -20.0f + std::fmod(taxiMotion * 0.92f + 20.0f, 40.0f);
    float frontBusX = -26.0f + std::fmod(bayBusPhase * 1.55f, 52.0f);

    // Front road sedan moving across the facade.
    glPushMatrix();
    glTranslatef(frontCarX, 0.0f, -18.6f);
    drawCar(0.72f,0.76f,0.80f);
    glPopMatrix();

    // Opposite taxi lane for lively urban feel.
    glPushMatrix();
    glTranslatef(18.0f - std::fmod(taxiMotion * 0.78f + 18.0f, 36.0f), 0.0f, -21.8f);
    glRotatef(180,0,1,0);
    drawTaxi(0.96f,0.78f,0.10f);
    glPopMatrix();

    // Distant bus crossing visible through the curtain-wall windows.
    glPushMatrix();
    glTranslatef(frontBusX, 0.0f, -27.0f);
    glScalef(0.86f,0.86f,0.86f);
    drawBus(0.84f,0.12f,0.10f);
    glPopMatrix();

    // V46 depth layer: smaller distant traffic creates outside depth feeling.
    glPushMatrix();
    glTranslatef(-12.0f,0.0f,-35.0f);
    glScalef(0.55f,0.55f,0.55f);
    drawMiniBus(0.78f,0.82f,0.86f);
    glPopMatrix();

    // V51: motorcycle removed from this view to avoid small detached-looking
    // geometry; the bus, taxi and sedan already provide clear exterior motion.
}

void drawV27RoadSurfaceGlow(){
    // Light pools are a night-only effect. During daytime even a low-alpha
    // overlay changes the intended black asphalt on some OpenGL 1.1 drivers.
    float night = clampf((dayNightBlend - 0.10f) / 0.90f, 0.0f, 1.0f);
    if (night <= 0.001f) return;
    drawLightPool(-6.0f, -14.0f, 1.9f, 1.0f, 0.82f, 0.48f, 0.065f*night);
    drawLightPool( 1.0f, -14.0f, 2.2f, 1.0f, 0.84f, 0.50f, 0.075f*night);
    drawLightPool( 8.0f, -14.2f, 1.9f, 1.0f, 0.82f, 0.48f, 0.065f*night);
    drawLightPool( 0.0f,   8.2f, 2.8f, 0.96f, 0.86f, 0.67f, 0.035f*night);
}

void drawV27TerminalNightGlow(){
    float night = clampf((dayNightBlend - 0.10f) / 0.90f, 0.0f, 1.0f);
    if (night <= 0.001f) return;

    // Glowing terminal facade bands and under-roof glow.
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float warmA = 0.18f*night;
    glColor4f(1.0f,0.78f,0.40f,warmA);
    glPushMatrix(); glTranslatef(2.6f,5.8f,7.3f); cube(22.0f,0.14f,0.08f); glPopMatrix();
    glPushMatrix(); glTranslatef(2.6f,4.6f,-1.0f); cube(20.0f,0.10f,0.08f); glPopMatrix();

    glColor4f(0.38f,0.72f,1.0f, 0.10f*night);
    glPushMatrix(); glTranslatef(2.6f,3.4f,8.05f); cube(23.0f,6.2f,0.03f); glPopMatrix();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // Ground glow pools.
    drawLightPool(-8.5f, 6.0f, 2.2f, 1.0f, 0.80f, 0.45f, 0.060f*night);
    drawLightPool( 0.0f, 6.5f, 2.7f, 1.0f, 0.82f, 0.48f, 0.075f*night);
    drawLightPool( 8.5f, 6.0f, 2.2f, 1.0f, 0.80f, 0.45f, 0.060f*night);
}

void drawV27CityLightingAccent(){
    float night = clampf((dayNightBlend - 0.10f) / 0.90f, 0.0f, 1.0f);
    if (night <= 0.001f) return;

    // Colored city-vibe accent lights around terminal approaches.
    drawLightPool(-22.0f, -20.0f, 1.3f, 0.95f, 0.26f, 0.18f, 0.045f*night);
    drawLightPool( 22.0f, -20.0f, 1.3f, 0.95f, 0.26f, 0.18f, 0.045f*night);
    drawLightPool(-22.0f,  20.0f, 1.3f, 0.20f, 0.85f, 0.30f, 0.050f*night);
    drawLightPool( 22.0f,  20.0f, 1.3f, 0.20f, 0.85f, 0.30f, 0.050f*night);
    drawLightPool(-9.0f, -29.0f, 1.8f, 0.42f, 0.52f, 0.70f, 0.025f*night);
    drawLightPool(23.5f, 0.0f, 1.9f, 1.0f, 0.46f, 0.18f, 0.040f*night);
}

void drawFinalCinematicCamera(){
    if(!cinematicTour) return;
    float t = std::fmod(cinematicTime, 52.0f);
    if(t < 10.0f){
        camX = 24.0f - t*1.5f;
        camY = 12.0f + 0.9f*std::sin(t*0.30f);
        camZ = 27.0f - t*1.0f;
        pointCameraAt(2.0f, 2.8f, 3.0f);
    } else if(t < 20.0f){
        float s=t-10.0f;
        camX = -22.0f + s*1.2f;
        camY = 3.0f;
        camZ = -18.0f;
        pointCameraAt(7.5f,1.8f,-18.0f);
    } else if(t < 30.0f){
        float s=t-20.0f;
        camX = 3.0f + 0.6f*std::sin(s*0.5f);
        camY = 2.4f;
        camZ = 10.0f - 0.2f*std::sin(s*0.3f);
        pointCameraAt(0.0f,1.55f,1.2f);
    } else if(t < 42.0f){
        float a=(t-30.0f)*0.22f;
        camX = 2.5f + 18.0f*std::cos(a);
        camY = 6.0f + 1.4f*std::sin(a*1.1f);
        camZ = 8.0f + 10.0f*std::sin(a);
        pointCameraAt(2.6f,3.0f,8.0f);
    } else {
        float s=t-42.0f;
        camX = -5.0f + s*1.2f;
        camY = 4.0f + 0.3f*std::sin(s*0.7f);
        camZ = -24.0f + s*0.5f;
        pointCameraAt(8.0f,1.5f,-18.0f);
    }
}


void timer(int) {
    fpsTimer += 16;
    if(fpsTimer >= 1000){
        fpsValue = fpsCounter;
        fpsCounter = 0;
        fpsTimer = 0;
    }
    trafficLightTimer += 0.05f;
    indicatorBlinkTimer += 0.08f;
    // Real sequence and useful durations: Red 5 s, Green 7 s, Yellow 2 s.
    const float signalDuration =
        (trafficLightState == 0) ? 5.0f :
        (trafficLightState == 1) ? 7.0f : 2.0f;
    if(trafficLightTimer > signalDuration){
        trafficLightTimer = 0.0f;
        trafficLightState = (trafficLightState + 1) % 3;
    }

    rainOffset += 0.35f;
    rainReflectionPulse += 0.08f;
    ledPulse += 0.12f;
    cloudDrift += 0.006f;
    if(cloudDrift > 30.0f) cloudDrift = -30.0f;
    birdDrift += 0.11f;
    if(birdDrift > 60.0f) birdDrift = -60.0f;
    birdFlapPhase += 14.0f;
    if(birdFlapPhase >= 360.0f) birdFlapPhase -= 360.0f;

    // Smooth wet-road transition: false starts from/returns to exactly zero.
    const float wetTarget = weatherRain ? 1.0f : 0.0f;
    roadWetness += (wetTarget - roadWetness) * (weatherRain ? 0.075f : 0.018f);
    if(!weatherRain && roadWetness < 0.001f) roadWetness = 0.0f;

    if (busDoorAngle < 70.0f) busDoorAngle += 0.2f;
    else busDoorAngle = 0.0f;

    crossingWalk += 0.06f;
    if(crossingWalk > 10.0f) crossingWalk = -5.0f;

    trafficFlowX += 0.07f;
    if(trafficFlowX>32.0f) trafficFlowX=-32.0f;

    if (std::fabs(dayNightBlend - dayNightTarget) > 0.001f) {
        dayNightBlend += (dayNightTarget - dayNightBlend) * 0.03f;
        if (std::fabs(dayNightBlend - dayNightTarget) < 0.002f) dayNightBlend = dayNightTarget;
    }

    if (animationOn) {
        windAngle += 1.45f;
        if (windAngle >= 360.0f) windAngle -= 360.0f;
        fanAngle += 3.8f;
        if (fanAngle >= 360.0f) fanAngle -= 360.0f;
    }

    if (cinematicTour) {
        cinematicTime += 0.016f;
        if (cinematicTime >= 52.0f) cinematicTime = 0.0f;
    }

    if (busMotionOn) {
        // V67: hold at red instead of always advancing through the signal.
        if (!trafficHeldAtSignal(movingBusX)) movingBusX += 0.060f;
        if (movingBusX > 24.0f) movingBusX = -24.0f;

        movingCarZ -= 0.075f;
        if (movingCarZ < -21.0f) movingCarZ = 21.0f;

        if (!trafficHeldAtSignal(taxiMotion)) taxiMotion += 0.08f;
        if (taxiMotion > 34.0f) taxiMotion = -34.0f;

        motorcycleMotion -= 0.12f;
        if (motorcycleMotion < -34.0f) motorcycleMotion = 34.0f;

        bayBusPhase += 0.06f;
        if (bayBusPhase > 34.0f) bayBusPhase = 0.0f;

        trafficSyncTime += 0.05f;
        trafficBrakePulse = 0.35f + 0.35f * std::sin(trafficSyncTime);
        // Keep buses, lights and traffic feeling synchronized.
        if (trafficSyncTime > 6.28f) trafficSyncTime = 0.0f;
        indicatorPulse += 0.08f;
        if(indicatorPulse > 1.0f) indicatorPulse = 0.0f;

        busWheelRotation -= 4.2f;
        if (busWheelRotation <= -360.0f) busWheelRotation += 360.0f;
    }

    // Stable ~60 FPS target timer; the optional C-key tour is the only
    // automatic camera path retained in V60.
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        // Viewing coordinate transformations.
        case 'w': case 'W': moveCamera(0.65f,0); break;
        case 's': case 'S': moveCamera(-0.65f,0); break;
        case 'a': case 'A': moveCamera(0,-0.65f); break;
        case 'd': case 'D': moveCamera(0, 0.65f); break;
        case 'q': case 'Q': if(cameraPreset != 3) camY += 0.45f; break;
        case 'e': case 'E': if(cameraPreset != 3) camY -= 0.45f; break;

        // Object coordinate transformations (the blue monument).
        case 'j': case 'J': objectTX -= 0.35f; break;
        case 'l': case 'L': objectTX += 0.35f; break;
        case 'i': case 'I': objectTZ -= 0.35f; break;
        case 'k': case 'K': objectTZ += 0.35f; break;
        case 'u': case 'U': objectRotY += 5.0f; break;
        case 'o': case 'O': objectRotY -= 5.0f; break;
        case '+': case '=': objectScale = std::min(2.5f, objectScale + 0.06f); break;
        case '-': case '_': objectScale = std::max(0.25f, objectScale - 0.06f); break;

        // Scene options.
        case 'r': case 'R': animationOn = !animationOn; break;
        case 'm': case 'M': busMotionOn = !busMotionOn; break;
        case 'n': case 'N': nightMode = !nightMode; dayNightTarget = nightMode ? 1.0f : 0.0f; break;
        case 'p': case 'P': weatherRain = !weatherRain; break;
        case 'h': case 'H': showHUD = !showHUD; break;
        case 'x': case 'X': showAxes = !showAxes; break;
        case 'c': case 'C':
            cinematicTour = !cinematicTour;
            if(cinematicTour){
                cameraPreset = 1; // Tour always uses the connected main world.
                cinematicTime = 0.0f;
            }
            break;

        // Camera presets.
        case '1': setCameraPreset(1); break;
        case '2': setCameraPreset(2); break;
        case '3': setCameraPreset(3); break;
        case '4': setCameraPreset(4); break;
        case '5': setCameraPreset(5); break;
        case '6': setCameraPreset(6); break;
        case '7': setCameraPreset(7); break;

        case 27: std::exit(0);
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int) {
    switch (key) {
        case GLUT_KEY_LEFT:  yawDeg -= 3.0f; break;
        case GLUT_KEY_RIGHT: yawDeg += 3.0f; break;
        case GLUT_KEY_UP:    pitchDeg = std::min(80.0f, pitchDeg + 2.5f); break;
        case GLUT_KEY_DOWN:  pitchDeg = std::max(-80.0f, pitchDeg - 2.5f); break;
    }
    glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON)   leftDragging = (state == GLUT_DOWN);
    if (button == GLUT_RIGHT_BUTTON)  rightDragging = (state == GLUT_DOWN);
    if (button == GLUT_MIDDLE_BUTTON) middleDragging = (state == GLUT_DOWN);

    // FreeGLUT/common GLUT mouse wheel: button 3=up, 4=down.
    if (state == GLUT_DOWN && (button == 3 || button == 4)) {
        int mods = glutGetModifiers();
        bool ctrl = (mods & GLUT_ACTIVE_CTRL) != 0;
        if (ctrl) {
            if (button == 3) objectScale = std::min(2.5f, objectScale + 0.08f);
            else             objectScale = std::max(0.25f, objectScale - 0.08f);
        } else {
            if (button == 3) moveCamera(0.85f,0);
            else             moveCamera(-0.85f,0);
        }
        glutPostRedisplay();
    }

    lastMouseX = x;
    lastMouseY = y;
}

void mouseMotion(int x, int y) {
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    if (leftDragging) {
        // Viewing-coordinate transformation using mouse.
        yawDeg += dx * 0.25f;
        pitchDeg = clampf(pitchDeg - dy * 0.20f, -80.0f, 80.0f);
    }

    if (rightDragging) {
        // Object-coordinate transformation using mouse.
        objectRotY += dx * 0.45f;
        objectTZ += dy * 0.018f;
    }

    if (middleDragging) {
        objectTX += dx * 0.018f;
        objectTY -= dy * 0.018f;
        objectTY = clampf(objectTY, -0.8f, 3.0f);
    }

    lastMouseX = x;
    lastMouseY = y;
    glutPostRedisplay();
}

// ============================================================================
// OpenGL initialization
// ============================================================================
void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);       // Full-scene edge anti-aliasing.
    glEnable(GL_NORMALIZE);         // Important because many objects are scaled.
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Optional quality features. Widely available in desktop OpenGL.
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    initTextures();
    buildStaticDisplayLists();      // Textured lists must be compiled after initTextures().
    setCameraPreset(1);
}

void printControls() {
    std::cout << "\n============================================================\n";
    std::cout << " NEXBUS URBAN HUB\n";
    std::cout << "============================================================\n";
    std::cout << "VIEW / CAMERA:\n";
    std::cout << "  W/S : forward/back       A/D : strafe left/right\n";
    std::cout << "  Q/E : up/down            Arrow keys : look\n";
    std::cout << "  Left mouse drag : look   Mouse wheel : move forward/back\n";
    std::cout << "  1 : overview  2 : bus hero  3 : locked interior/window view\n";
    std::cout << "  4 : road traffic  5 : passenger bay  6 : elevated city  7 : facade\n";
    std::cout << "  Note: translation is locked in preset 3; look controls still work.\n\n";
    std::cout << "OBJECT TRANSFORMATION (blue monument):\n";
    std::cout << "  I/K : Z translation      J/L : X translation\n";
    std::cout << "  U/O : Y rotation         +/- : scale\n";
    std::cout << "  Right drag : rotate + Z translate\n";
    std::cout << "  Middle drag : X/Y translate\n";
    std::cout << "  Ctrl + wheel : scale object\n\n";
    std::cout << "SCENE:\n";
    std::cout << "  R : rotate/pause windmill + ceiling fans + roof vents\n";
    std::cout << "  M : pause/resume all vehicle traffic\n";
    std::cout << "  N : day/night   P : rain/dry   C : start/stop cinematic tour\n";
    std::cout << "  H : HUD         X : axes       ESC : exit\n";
    std::cout << "============================================================\n\n";
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(winW, winH);
    glutInitWindowPosition(60, 30);
    glutCreateWindow("CSE 4208 - NEXBUS URBAN HUB V69 Clear Glass + Birds");

    initGL();
    printControls();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
