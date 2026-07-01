#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include <cmath>
#include <cstring>
#include <vector>
#include "executor.h"
#include "block.h"

static GLUquadric* s_quad = nullptr;

// ---------------------------------------------------------------------------
// Texturas procedurais
// ---------------------------------------------------------------------------
static GLuint s_textures[4] = {0,0,0,0};
static const char* const s_texNames[4] = {"madeira","tijolo","xadrez","metal"};

static void generateTexture(int idx, GLuint id)
{
    const int S = 64;
    static unsigned char px[64*64*3];

    for (int y = 0; y < S; ++y) {
        for (int x = 0; x < S; ++x) {
            int b = (y*S+x)*3;
            switch (idx) {
            case 0: { // madeira — listras com ruído suave
                float v = sinf((x + y*0.25f)*0.4f + cosf(x*0.12f)*1.8f)*0.5f + 0.5f;
                px[b+0]=(unsigned char)(105+v*90); px[b+1]=(unsigned char)(58+v*42); px[b+2]=(unsigned char)(18+v*18);
                break; }
            case 1: { // tijolo — grade com argamassa
                int row=y/16; int bx=(row%2==1)?(x+8)%S:x;
                bool mortar=(y%16<2)||(bx%16<2);
                int var=(bx/16+row)%3;
                px[b+0]=mortar?155:(175+var*8); px[b+1]=mortar?145:(72+var*5); px[b+2]=mortar?135:(52+var*4);
                break; }
            case 2: { // xadrez
                unsigned char c=((x/8+y/8)%2==0)?215:40;
                px[b+0]=px[b+1]=px[b+2]=c;
                break; }
            case 3: { // metal — reflexo suave
                float v=(sinf(x*0.75f+y*0.3f)+cosf(x*0.28f-y*0.55f))*0.25f+0.75f;
                unsigned char c=(unsigned char)(125+v*100);
                px[b+0]=c; px[b+1]=c; px[b+2]=(unsigned char)(c*0.87f);
                break; }
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, S, S, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

// ---------------------------------------------------------------------------
// Gizmos de luz — desenhados em world-space, iluminação desligada
// ---------------------------------------------------------------------------
static void drawPointLightGizmo(float x, float y, float z)
{
    glPushMatrix();
    glTranslatef(x, y, z);

    glColor3f(1.f, 1.f, 0.65f);
    gluSphere(s_quad, 0.07f, 10, 10);

    // 6 raios ±X ±Y ±Z
    const float r = 0.20f;
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex3f(0,0,0); glVertex3f( r, 0, 0);
        glVertex3f(0,0,0); glVertex3f(-r, 0, 0);
        glVertex3f(0,0,0); glVertex3f(0,  r, 0);
        glVertex3f(0,0,0); glVertex3f(0, -r, 0);
        glVertex3f(0,0,0); glVertex3f(0, 0,  r);
        glVertex3f(0,0,0); glVertex3f(0, 0, -r);
    glEnd();
    glLineWidth(1.f);

    glPopMatrix();

    // Linha tracejada até o piso (y=0)
    if (y > 0.01f) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0xAAAA);
        glColor3f(1.f, 0.85f, 0.3f);
        glBegin(GL_LINES);
            glVertex3f(x, y, z);
            glVertex3f(x, 0.f, z);
        glEnd();
        glDisable(GL_LINE_STIPPLE);
    }
}

static void drawDirLightGizmo(float dx, float dy, float dz)
{
    float len = sqrtf(dx*dx + dy*dy + dz*dz);
    if (len < 0.001f) return;
    dx /= len; dy /= len; dz /= len;

    // 3 setas paralelas deslocadas em X
    const float arrowLen = 1.1f;
    const float ox[3] = { -0.4f, 0.f, 0.4f };

    glLineWidth(2.f);
    glColor3f(1.f, 0.95f, 0.4f);

    for (int i = 0; i < 3; ++i) {
        // Origem: atrás e acima da direção da luz
        float sx = ox[i] - dx * 1.8f;
        float sy =        - dy * 1.8f + 2.2f;
        float sz =        - dz * 1.8f;
        float ex = sx + dx * arrowLen;
        float ey = sy + dy * arrowLen;
        float ez = sz + dz * arrowLen;

        glBegin(GL_LINES);
            glVertex3f(sx, sy, sz);
            glVertex3f(ex, ey, ez);
        glEnd();

        glPushMatrix();
            glTranslatef(ex, ey, ez);
            glColor3f(1.f, 0.85f, 0.2f);
            gluSphere(s_quad, 0.05f, 8, 8);
        glPopMatrix();
        glColor3f(1.f, 0.95f, 0.4f);
    }
    glLineWidth(1.f);
}

void executorInit()
{
    s_quad = gluNewQuadric();
    gluQuadricNormals(s_quad, GLU_SMOOTH);
    gluQuadricTexture(s_quad, GLU_TRUE);

    glGenTextures(4, s_textures);
    for (int i = 0; i < 4; ++i)
        generateTexture(i, s_textures[i]);
}

void executorCleanup()
{
    if (s_quad) {
        gluDeleteQuadric(s_quad);
        s_quad = nullptr;
    }
    glDeleteTextures(4, s_textures);
}

void executorRun(const std::vector<Block>& script,
                 int vpW, int vpH,
                 float camRotX, float camRotY)
{
    // --- 1. Reseta estado crítico ---
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glEnable(GL_DEPTH_TEST);
    glColor3f(1.f, 1.f, 1.f);

    // --- 2. Projeção ---
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = (vpH > 0) ? (double)vpW / vpH : 1.0;

    bool foundCam = false;
    for (const Block& b : script) {
        if (!b.enabled) continue;
        if (b.type == BLK_CAM_PERSPECTIVE) {
            float fov     = b.params[0] > 0  ? b.params[0] : 60.f;
            float nearVal = b.params[1] > 0  ? b.params[1] : 0.1f;
            float farVal  = b.params[2] > 10 ? b.params[2] : 50.f;
            gluPerspective(fov, aspect, nearVal, farVal);
            foundCam = true; break;
        }
        if (b.type == BLK_CAM_ORTHO) {
            float zoom = b.params[0] > 0 ? b.params[0] : 3.f;
            glOrtho(-zoom * aspect, zoom * aspect, -zoom, zoom, 0.1, 100.0);
            foundCam = true; break;
        }
    }
    if (!foundCam)
        gluPerspective(60.0, aspect, 0.1, 50.0);

    // --- 3. Modelview + LookAt + câmera orbital ---
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    bool foundLookAt = false;
    for (const Block& b : script) {
        if (!b.enabled) continue;
        if (b.type == BLK_CAM_LOOKAT) {
            gluLookAt(b.params[0], b.params[1], b.params[2],
                      b.params[3], b.params[4], b.params[5],
                      0.0, 1.0, 0.0);
            foundLookAt = true; break;
        }
    }
    if (!foundLookAt)
        gluLookAt(0.0, 0.0, 5.0,
                  0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0);

    glRotatef(camRotX, 1.0f, 0.0f, 0.0f);
    glRotatef(camRotY, 0.0f, 1.0f, 0.0f);

    // Salva world-space para restaurar ao desenhar gizmos de luz
    glPushMatrix();

    // --- 4. Iluminação desligada por padrão ---
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);

    // --- 5. Depth test ---
    glEnable(GL_DEPTH_TEST);

    // --- 6. Grid no plano XZ (depth mask false → não obstrui objetos) ---
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glColor3f(0.65f, 0.67f, 0.74f);
    glBegin(GL_LINES);
    for (int i = -5; i <= 5; ++i) {
        glVertex3f((float)i, 0.f, -5.f);
        glVertex3f((float)i, 0.f,  5.f);
        glVertex3f(-5.f, 0.f, (float)i);
        glVertex3f( 5.f, 0.f, (float)i);
    }
    glEnd();

    // --- 7. Eixos de referência ---
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glColor3f(0.80f, 0.15f, 0.15f); glVertex3f(0,0,0); glVertex3f(1.5f,0,0);
        glColor3f(0.15f, 0.72f, 0.20f); glVertex3f(0,0,0); glVertex3f(0,1.5f,0);
        glColor3f(0.20f, 0.30f, 0.85f); glVertex3f(0,0,0); glVertex3f(0,0,1.5f);
    glEnd();
    glLineWidth(1.f);

    // --- 8. Re-habilita depth mask para objetos ---
    glDepthMask(GL_TRUE);

    // --- 9. Execução sequencial dos blocos ---
    int pushDepth = 0;

    for (const Block& b : script) {
        if (!b.enabled) continue;

        switch (b.type) {

        // Câmera: já tratada acima, pula
        case BLK_CAM_PERSPECTIVE:
        case BLK_CAM_ORTHO:
        case BLK_CAM_LOOKAT:
            break;

        // ---- Transformações ----
        case BLK_TRANSLATE:
            glTranslatef(b.params[0], b.params[1], b.params[2]);
            break;

        case BLK_ROTATE:
            glRotatef(b.params[0], b.params[1], b.params[2], b.params[3]);
            break;

        case BLK_SCALE:
            glScalef(b.params[0] > 0 ? b.params[0] : 1.f,
                     b.params[1] > 0 ? b.params[1] : 1.f,
                     b.params[2] > 0 ? b.params[2] : 1.f);
            break;

        case BLK_PUSH_POP:
            glPushMatrix();
            pushDepth++;
            break;

        // ---- Objetos ----
        case BLK_CUBE: {
            float size = b.params[0] > 0 ? b.params[0] : 1.f;
            glutSolidCube(size);
            break;
        }

        case BLK_SPHERE: {
            float r = b.params[0] > 0  ? b.params[0] : 0.7f;
            int  sl = (int)b.params[1]; if (sl < 4) sl = 16;
            int  st = (int)b.params[2]; if (st < 4) st = 16;
            gluSphere(s_quad, r, sl, st);
            break;
        }

        case BLK_TEAPOT: {
            float size = b.params[0] > 0 ? b.params[0] : 1.f;
            glutSolidTeapot(size);
            break;
        }

        case BLK_PLANE: {
            float hw = (b.params[0] > 0 ? b.params[0] : 2.f) * 0.5f;
            float hd = (b.params[1] > 0 ? b.params[1] : 2.f) * 0.5f;
            // Para o plano horizontal, GL_OBJECT_LINEAR (S=X, T=Z) é mais natural
            bool texOn = (glIsEnabled(GL_TEXTURE_2D) == GL_TRUE);
            if (texOn) {
                float sp[] = {1.f/(2.f*hw), 0.f, 0.f, 0.5f};
                float tp[] = {0.f, 0.f, 1.f/(2.f*hd), 0.5f};
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                glTexGenfv(GL_S, GL_OBJECT_PLANE, sp);
                glTexGenfv(GL_T, GL_OBJECT_PLANE, tp);
            }
            glBegin(GL_QUADS);
                glNormal3f(0.f, 1.f, 0.f);
                glVertex3f(-hw, 0.f, -hd);
                glVertex3f( hw, 0.f, -hd);
                glVertex3f( hw, 0.f,  hd);
                glVertex3f(-hw, 0.f,  hd);
            glEnd();
            // Restaura sphere map para próximos objetos
            if (texOn) {
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            }
            break;
        }

        case BLK_CONE: {
            float base   = b.params[0] > 0 ? b.params[0] : 0.5f;
            float height = b.params[1] > 0 ? b.params[1] : 1.f;
            int   slices = (int)b.params[2]; if (slices < 4) slices = 16;
            gluCylinder(s_quad, base, 0.0, height, slices, 4);
            glPushMatrix();
                glRotatef(180.f, 1.f, 0.f, 0.f);
                gluDisk(s_quad, 0.0, base, slices, 1);
            glPopMatrix();
            break;
        }

        case BLK_CYLINDER: {
            float r      = b.params[0] > 0 ? b.params[0] : 0.4f;
            float height = b.params[1] > 0 ? b.params[1] : 1.f;
            int   slices = (int)b.params[2]; if (slices < 4) slices = 16;
            gluCylinder(s_quad, r, r, height, slices, 4);
            glPushMatrix();
                glRotatef(180.f, 1.f, 0.f, 0.f);
                gluDisk(s_quad, 0.0, r, slices, 1);
            glPopMatrix();
            glPushMatrix();
                glTranslatef(0.f, 0.f, height);
                gluDisk(s_quad, 0.0, r, slices, 1);
            glPopMatrix();
            break;
        }

        // ---- Iluminação ----
        case BLK_LIGHT_ENABLE: {
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            glEnable(GL_NORMALIZE);
            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            float white[4] = {1.f, 1.f, 1.f, 1.f};
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  white);
            glLightfv(GL_LIGHT0, GL_SPECULAR, white);
            break;
        }

        case BLK_LIGHT_DIR: {
            float inten = b.params[3] > 0 ? b.params[3] : 1.f;
            float pos[4] = { b.params[0], b.params[1], b.params[2], 0.f };
            float dif[4] = { inten, inten, inten, 1.f };
            glLightfv(GL_LIGHT0, GL_POSITION, pos);
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
            break;
        }

        case BLK_LIGHT_POINT: {
            float inten = b.params[3] > 0 ? b.params[3] : 1.f;
            float pos[4] = { b.params[0], b.params[1], b.params[2], 1.f };
            float dif[4] = { inten, inten, inten, 1.f };
            glLightfv(GL_LIGHT0, GL_POSITION, pos);
            glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
            break;
        }

        case BLK_MATERIAL: {
            float diffuse[4]  = { b.params[0], b.params[1], b.params[2], 1.f };
            float specular[4] = { 0.5f, 0.5f, 0.5f, 1.f };
            float shininess   = b.params[3] > 0 ? b.params[3] : 32.f;
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffuse);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
            glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, shininess);
            break;
        }

        case BLK_AMBIENT_COLOR: {
            float amb[4] = { b.params[0], b.params[1], b.params[2], 1.f };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);
            break;
        }

        // ---- Textura ----
        case BLK_TEXTURE: {
            GLuint tid = s_textures[2]; // fallback: xadrez
            for (int t = 0; t < 4; ++t)
                if (strcmp(b.strParam, s_texNames[t]) == 0) { tid = s_textures[t]; break; }
            glBindTexture(GL_TEXTURE_2D, tid);
            glEnable(GL_TEXTURE_2D);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            // Sphere mapping: gera coords automáticas em todos os primitivos
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            break;
        }

        // ---- Curva de Bezier ----
        case BLK_BEZIER: {
            float p[4][3] = {
                { b.params[0], b.params[1], 0.f },
                { b.params[2], b.params[3], 0.f },
                { b.params[4], b.params[5], 0.f },
                { 1.5f,        0.f,         0.f }
            };
            int steps = 50;
            glDisable(GL_LIGHTING);
            glLineWidth(2.5f);
            glColor3f(0.3f, 0.85f, 0.5f);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= steps; ++i) {
                float t = i / (float)steps;
                float u = 1.f - t;
                float x = u*u*u*p[0][0] + 3*u*u*t*p[1][0] + 3*u*t*t*p[2][0] + t*t*t*p[3][0];
                float y = u*u*u*p[0][1] + 3*u*u*t*p[1][1] + 3*u*t*t*p[2][1] + t*t*t*p[3][1];
                glVertex3f(x, y, 0.f);
            }
            glEnd();
            glLineWidth(1.f);
            glPointSize(6.f);
            glColor3f(1.f, 1.f, 0.5f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 4; ++i) glVertex3fv(p[i]);
            glEnd();
            glPointSize(1.f);
            glColor3f(0.5f, 0.5f, 0.5f);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i < 4; ++i) glVertex3fv(p[i]);
            glEnd();
            break;
        }

        default:
            break;
        }
    }

    // Pop de todos os glPushMatrix pendentes da cena
    while (pushDepth-- > 0)
        glPopMatrix();

    // Restaura world-space (cancela transforms acumulados da cena)
    glPopMatrix();

    // --- 10. Gizmos de luz (world-space, sem iluminação) ---
    glDisable(GL_LIGHTING);

    for (const Block& b : script) {
        if (!b.enabled) continue;
        if (b.type == BLK_LIGHT_POINT)
            drawPointLightGizmo(b.params[0], b.params[1], b.params[2]);
        else if (b.type == BLK_LIGHT_DIR)
            drawDirLightGizmo(b.params[0], b.params[1], b.params[2]);
    }
}
