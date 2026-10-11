// Smart Traffic Congestion and Signal Simulation
#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
using namespace std;

const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 700;
const int MAX_CARS_PER_DIRECTION = 8;
const float ROAD_HALF_WIDTH = 125.0f;
const float STOP_DISTANCE = ROAD_HALF_WIDTH + 45.0f;

enum SignalState { NS_GREEN = 0, NS_YELLOW = 1, EW_GREEN = 2, EW_YELLOW = 3 };
enum TrafficLevel { LOW_TRAFFIC = 1, MEDIUM_TRAFFIC = 2, HEAVY_TRAFFIC = 3 };

struct Vehicle {
    float position;
    float speed;
    float r, g, b;
    bool passed;
};

Vehicle northSouth[MAX_CARS_PER_DIRECTION];
Vehicle eastWest[MAX_CARS_PER_DIRECTION];

int trafficLevel = MEDIUM_TRAFFIC;
int signalState = NS_GREEN;
bool running = true;
bool adaptiveMode = true;
bool mouseSelected = false;
float signalTimer = 0.0f;
int vehiclesPassed = 0;
int windowWidth = WINDOW_WIDTH;
int windowHeight = WINDOW_HEIGHT;

// Vertex array: one reusable unit rectangle for cars and UI buttons.
const GLfloat UNIT_QUAD[] = {
    -1.0f, -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,  1.0f
};

void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; ++c) glutBitmapCharacter(font, *c);
}

void drawRectangle(float x, float y, float width, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void drawVertexArrayRect(float x, float y, float width, float height, float r, float g, float b) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, UNIT_QUAD);
    glPushMatrix();
    glTranslatef(x + width / 2.0f, y + height / 2.0f, 0.0f);
    glScalef(width / 2.0f, height / 2.0f, 1.0f);
    glColor3f(r, g, b);
    glDrawArrays(GL_QUADS, 0, 4);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

void drawCircle(float cx, float cy, float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 32; ++i) {
        float angle = 2.0f * 3.1415926f * i / 32.0f;
        glVertex2f(cx + cos(angle) * radius, cy + sin(angle) * radius);
    }
    glEnd();
}

void drawRoad() {
    drawRectangle(0, 0, 1000, 700, 0.20f, 0.55f, 0.28f);
    drawRectangle(0, 250, 1000, 200, 0.12f, 0.12f, 0.14f);
    drawRectangle(375, 0, 250, 700, 0.12f, 0.12f, 0.14f);

    glColor3f(0.95f, 0.85f, 0.20f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    for (int x = 20; x < 370; x += 55) { glVertex2f(x, 348); glVertex2f(x + 28, 348); }
    for (int x = 650; x < 980; x += 55) { glVertex2f(x, 348); glVertex2f(x + 28, 348); }
    for (int y = 20; y < 245; y += 55) { glVertex2f(498, y); glVertex2f(498, y + 28); }
    for (int y = 455; y < 680; y += 55) { glVertex2f(498, y); glVertex2f(498, y + 28); }
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 6; ++i) {
        glVertex2f(530 + i * 12, 235); glVertex2f(530 + i * 12, 250); // bottom horizontal pedestrian crossing
        glVertex2f(400 + i * 12, 235); glVertex2f(400 + i * 12, 250); // bottom horizontal pedestrian crossing
        glVertex2f(530 + i * 12, 450); glVertex2f(530 + i * 12, 465); // top horizontal pedestrian crossing
        glVertex2f(400 + i * 12, 450); glVertex2f(400 + i * 12, 465); // top horizontal pedestrian crossing
        glVertex2f(625, 380 + i * 12); glVertex2f(635, 380 + i * 12); // right vertical pedestrian crossing
        glVertex2f(625, 275 + i * 12); glVertex2f(635, 275 + i * 12); // right vertical pedestrian crossing
        glVertex2f(365, 380 + i * 12); glVertex2f(375, 380 + i * 12); // left vertical pedestrian crossing
        glVertex2f(365, 270 + i * 12); glVertex2f(375, 270 + i * 12); // left vertical pedestrian crossing
    }
    glEnd();
}

void drawVehicle(const Vehicle& car, bool ns, int index) {
    float x = ns ? 450.0f + (index % 2) * 54.0f : car.position;
    float y = ns ? car.position : 300.0f + (index % 2) * 54.0f;
    if (ns) drawVertexArrayRect(x, y, 35, 62, car.r, car.g, car.b);
    else drawVertexArrayRect(x, y, 62, 35, car.r, car.g, car.b);

    glColor3f(0.65f, 0.85f, 0.95f);
    if (ns) drawRectangle(x + 6, y + 34, 23, 16, 0.65f, 0.85f, 0.95f);
    else drawRectangle(x + 34, y + 6, 16, 23, 0.65f, 0.85f, 0.95f);
}

void drawTrafficSignal(float x, float y, bool ns) {
    bool activeGreen = ns ? (signalState == NS_GREEN) : (signalState == EW_GREEN);
    bool activeYellow = ns ? (signalState == NS_YELLOW) : (signalState == EW_YELLOW);
    drawRectangle(x, y, 42, 118, 0.04f, 0.04f, 0.05f);
    drawCircle(x + 21, y + 88, 13, activeGreen ? 0.15f : 0.25f, activeGreen ? 0.85f : 0.05f, 0.15f);
    drawCircle(x + 21, y + 59, 13, activeYellow ? 0.95f : 0.25f, activeYellow ? 0.75f : 0.20f, 0.05f);
    drawCircle(x + 21, y + 30, 13, (!activeGreen && !activeYellow) ? 0.90f : 0.25f, 0.05f, 0.05f);
}

int waitingCount(const Vehicle* cars) {
    int count = 0;
    int total = trafficLevel * 2 + 2;
    for (int i = 0; i < total; ++i) if (!cars[i].passed) ++count;
    return count;
}

const char* signalName() {
    if (signalState == NS_GREEN) return "N/S GREEN";
    if (signalState == NS_YELLOW) return "N/S YELLOW";
    if (signalState == EW_GREEN) return "E/W GREEN";
    return "E/W YELLOW";
}

void setSignal(int next) { signalState = next % 4; signalTimer = 0.0f; }

void updateSignal(float dt) {
    int nsWaiting = waitingCount(northSouth);
    int ewWaiting = waitingCount(eastWest);
    float greenDuration = 6.0f;
    if (adaptiveMode && signalState == NS_GREEN && nsWaiting > ewWaiting) greenDuration = 9.0f;
    if (adaptiveMode && signalState == EW_GREEN && ewWaiting > nsWaiting) greenDuration = 9.0f;
    float duration = (signalState == NS_GREEN || signalState == EW_GREEN) ? greenDuration : 2.0f;
    signalTimer += dt;
    if (signalTimer >= duration) setSignal(signalState + 1);
}

void updateVehicles(float dt) {
    int total = trafficLevel * 2 + 2;
    bool nsCanMove = signalState == NS_GREEN;
    bool ewCanMove = signalState == EW_GREEN;
    for (int i = 0; i < total; ++i) {
        if (nsCanMove || northSouth[i].position < STOP_DISTANCE - i * 78) northSouth[i].position += northSouth[i].speed * dt;
        if (ewCanMove || eastWest[i].position < STOP_DISTANCE - i * 78) eastWest[i].position += eastWest[i].speed * dt;
        if (northSouth[i].position > 760.0f && !northSouth[i].passed) { northSouth[i].passed = true; ++vehiclesPassed; }
        if (eastWest[i].position > 1040.0f && !eastWest[i].passed) { eastWest[i].passed = true; ++vehiclesPassed; }
        if (northSouth[i].position > 760.0f) northSouth[i].position = -130.0f - i * 78;
        if (eastWest[i].position > 1040.0f) eastWest[i].position = -130.0f - i * 78;
    }
}

void resetSimulation() {
    trafficLevel = MEDIUM_TRAFFIC;
    signalState = NS_GREEN;
    signalTimer = 0.0f;
    vehiclesPassed = 0;
    running = true;
    adaptiveMode = true;
    for (int i = 0; i < MAX_CARS_PER_DIRECTION; ++i) {
        northSouth[i] = { -110.0f - i * 78.0f, 48.0f, 0.85f, 0.25f + i * 0.05f, 0.20f, false };
        eastWest[i] = { -110.0f - i * 78.0f, 55.0f, 0.20f, 0.45f, 0.90f - i * 0.05f, false };
    }
}

void drawPanel() {
    drawRectangle(18, 520, 300, 155, 0.03f, 0.04f, 0.07f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(34, 650, "SMART TRAFFIC SYSTEM", GLUT_BITMAP_HELVETICA_18);
    char line[120];
    snprintf(line, sizeof(line), "Traffic level: %s", trafficLevel == LOW_TRAFFIC ? "LOW" : trafficLevel == MEDIUM_TRAFFIC ? "MEDIUM" : "HEAVY");
    drawText(34, 625, line);
    snprintf(line, sizeof(line), "N/S waiting: %d vehicles", waitingCount(northSouth)); drawText(34, 605, line);
    snprintf(line, sizeof(line), "E/W waiting: %d vehicles", waitingCount(eastWest)); drawText(34, 585, line);
    snprintf(line, sizeof(line), "Active signal: %s", signalName()); drawText(34, 565, line);
    snprintf(line, sizeof(line), "Mode: %s | Passed: %d", adaptiveMode ? "ADAPTIVE" : "MANUAL", vehiclesPassed); drawText(34, 545, line);

    drawRectangle(690, 540, 290, 125, 0.03f, 0.04f, 0.07f);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(710, 640, "CONTROLS", GLUT_BITMAP_HELVETICA_18);
    drawText(710, 620, "1/2/3: traffic level");
    drawText(710, 600, "A: adaptive/manual");
    drawText(710, 580, "N: next signal | SPACE: pause");
    drawText(710, 560, "R: reset | click signal | ESC: exit");
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawRoad();
    drawTrafficSignal(320, 470, false);
    drawTrafficSignal(638, 470, true);
    drawTrafficSignal(320, 112, true);
    drawTrafficSignal(638, 112, false);
    int total = trafficLevel * 2 + 2;
    for (int i = 0; i < total; ++i) { drawVehicle(northSouth[i], true, i); drawVehicle(eastWest[i], false, i); }
    drawPanel();
    glutSwapBuffers();
}

void timer(int) {
    static int previous = 0;
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = previous == 0 ? 0.016f : (now - previous) / 1000.0f;
    previous = now;
    dt = min(dt, 0.05f);
    if (running) { updateSignal(dt); updateVehicles(dt); }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int, int) {
    if (key == 27) exit(0);
    if (key == '1') trafficLevel = LOW_TRAFFIC;
    else if (key == '2') trafficLevel = MEDIUM_TRAFFIC;
    else if (key == '3') trafficLevel = HEAVY_TRAFFIC;
    else if (key == 'a' || key == 'A') adaptiveMode = !adaptiveMode;
    else if (key == 'n' || key == 'N') setSignal(signalState + 1);
    else if (key == ' ') running = !running;
    else if (key == 'r' || key == 'R') resetSimulation();
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;
    float wx = static_cast<float>(x) * 1000.0f / windowWidth;
    float wy = 700.0f - static_cast<float>(y) * 700.0f / windowHeight;
    // Clicking any visible signal selects it and advances the signal in manual mode.
    bool clickedSignal = (wx >= 300 && wx <= 700 && ((wy >= 100 && wy <= 600)));
    if (clickedSignal && !adaptiveMode) { mouseSelected = true; setSignal(signalState + 1); }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    windowWidth = max(1, w); windowHeight = max(1, h);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0, 1000, 0, 700);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Smart Traffic Congestion and Signal Simulation");
    glClearColor(0.08f, 0.12f, 0.20f, 1.0f);
    resetSimulation();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
