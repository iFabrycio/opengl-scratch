#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <vector>
#include <cstdio>
#include <cstring>
#include "config.h"
#include "state.h"
#include "ui/panel.h"
#include "ui/palette.h"
#include "ui/script.h"
#include "engine/block.h"
#include "engine/executor.h"
#include "engine/codegen.h"
#include <string>
#include <sstream>

// ---------------------------------------------------------------------------
// Definicoes das variaveis externas declaradas em state.h
// ---------------------------------------------------------------------------
std::vector<Block> g_script;
int  g_selectedBlock = -1;
bool g_needsRedraw   = true;

// ---------------------------------------------------------------------------
// Câmera orbital — drag no viewport 3D para rotacionar a cena
// ---------------------------------------------------------------------------
static float s_camRotX  =  20.0f;
static float s_camRotY  = -30.0f;
static bool  s_dragging =  false;
static int   s_dragLastX = 0;
static int   s_dragLastY = 0;
static int   s_codeScroll     = 0;
static int   s_codeScrollMax  = 0;
static bool  s_codeScrollDrag = false;

static void copyCodeToClipboard()
{
    std::string code = codegenRun(g_script);
    if (code.empty()) return;
#ifdef __APPLE__
    FILE* p = popen("pbcopy", "w");
#elif defined(_WIN32)
    FILE* p = popen("clip", "w");
#else
    FILE* p = popen("xclip -selection clipboard 2>/dev/null || xsel --clipboard --input", "w");
#endif
    if (p) { fwrite(code.c_str(), 1, code.size(), p); pclose(p); }
}

// ---------------------------------------------------------------------------
// addBlockToScript — callback registrado na paleta
// ---------------------------------------------------------------------------
void addBlockToScript(BlockType t)
{
    g_script.push_back(makeBlock(t));
    g_needsRedraw = true;
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// loadPreset — carrega um script pré-definido
// ---------------------------------------------------------------------------
static void loadPreset(int n)
{
    g_script.clear();
    g_selectedBlock = -1;

    switch (n) {
    case 1: { // Cubo com Luz
        g_script.push_back(makeBlock(BLK_LIGHT_ENABLE));
        Block ld = makeBlock(BLK_LIGHT_DIR);
        ld.params[0]=0.5f; ld.params[1]=1.f; ld.params[2]=0.5f; ld.params[3]=1.f;
        g_script.push_back(ld);
        Block mat = makeBlock(BLK_MATERIAL);
        mat.params[0]=0.2f; mat.params[1]=0.5f; mat.params[2]=1.f; mat.params[3]=64.f;
        g_script.push_back(mat);
        g_script.push_back(makeBlock(BLK_CUBE));
        break;
    }
    case 2: { // Esfera Texturizada com Luz
        g_script.push_back(makeBlock(BLK_LIGHT_ENABLE));
        Block lp = makeBlock(BLK_LIGHT_POINT);
        lp.params[0]=2.f; lp.params[1]=3.f; lp.params[2]=2.f; lp.params[3]=1.f;
        g_script.push_back(lp);
        Block tex = makeBlock(BLK_TEXTURE);
        strncpy(tex.strParam, "xadrez", sizeof(tex.strParam)-1);
        g_script.push_back(tex);
        Block mat = makeBlock(BLK_MATERIAL);
        mat.params[0]=1.f; mat.params[1]=1.f; mat.params[2]=1.f; mat.params[3]=96.f;
        g_script.push_back(mat);
        g_script.push_back(makeBlock(BLK_SPHERE));
        break;
    }
    case 3: { // Sistema solar simples
        g_script.push_back(makeBlock(BLK_LIGHT_ENABLE));
        Block lp = makeBlock(BLK_LIGHT_POINT);
        lp.params[0]=0.f; lp.params[1]=0.f; lp.params[2]=0.f; lp.params[3]=1.f;
        g_script.push_back(lp);
        // Sol
        Block amb = makeBlock(BLK_AMBIENT_COLOR);
        amb.params[0]=0.3f; amb.params[1]=0.3f; amb.params[2]=0.1f;
        g_script.push_back(amb);
        Block mat1 = makeBlock(BLK_MATERIAL);
        mat1.params[0]=1.f; mat1.params[1]=0.8f; mat1.params[2]=0.f; mat1.params[3]=10.f;
        g_script.push_back(mat1);
        Block sun = makeBlock(BLK_SPHERE);
        sun.params[0]=0.45f; sun.params[1]=20.f; sun.params[2]=20.f;
        g_script.push_back(sun);
        // Planeta azul
        Block tr = makeBlock(BLK_TRANSLATE);
        tr.params[0]=2.f; tr.params[1]=0.f; tr.params[2]=0.f;
        g_script.push_back(tr);
        Block mat2 = makeBlock(BLK_MATERIAL);
        mat2.params[0]=0.2f; mat2.params[1]=0.5f; mat2.params[2]=1.f; mat2.params[3]=48.f;
        g_script.push_back(mat2);
        Block planet = makeBlock(BLK_SPHERE);
        planet.params[0]=0.22f; planet.params[1]=16.f; planet.params[2]=16.f;
        g_script.push_back(planet);
        break;
    }
    case 4: { // Bule texturizado
        g_script.push_back(makeBlock(BLK_LIGHT_ENABLE));
        Block ld = makeBlock(BLK_LIGHT_DIR);
        ld.params[0]=0.6f; ld.params[1]=1.f; ld.params[2]=0.4f; ld.params[3]=1.f;
        g_script.push_back(ld);
        Block tex = makeBlock(BLK_TEXTURE);
        strncpy(tex.strParam, "madeira", sizeof(tex.strParam)-1);
        g_script.push_back(tex);
        Block mat = makeBlock(BLK_MATERIAL);
        mat.params[0]=1.f; mat.params[1]=1.f; mat.params[2]=1.f; mat.params[3]=32.f;
        g_script.push_back(mat);
        g_script.push_back(makeBlock(BLK_TEAPOT));
        break;
    }
    }
    g_needsRedraw = true;
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// beginPanel — configura viewport + projecao para um painel da tela
// ---------------------------------------------------------------------------
void beginPanel(int x, int y, int w, int h, bool is2D)
{
    glViewport(x, y, w, h);
    glScissor(x, y, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (is2D)
        glOrtho(0, w, h, 0, -1, 1);   // Y=0 no topo, igual mouse GLUT
    else
        gluPerspective(60.0, (double)w / h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (is2D) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
    }
}

// ---------------------------------------------------------------------------
// display
// ---------------------------------------------------------------------------
void display()
{
    glClearColor(VP_BG_R, VP_BG_G, VP_BG_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    int vpW = WIN_W - PALETTE_W - SCRIPT_W;
    int vpH = WIN_H - FOOTER_H;

    // --- Paleta ---
    beginPanel(0, FOOTER_H, PALETTE_W, vpH, true);
    paletteDraw(PALETTE_W, vpH);

    // --- Script ---
    beginPanel(PALETTE_W, FOOTER_H, SCRIPT_W, vpH, true);
    scriptDraw(SCRIPT_W, vpH);

    // --- Viewport 3D ---
    beginPanel(PALETTE_W + SCRIPT_W, FOOTER_H, vpW, vpH, false);
    glClearColor(VP_BG_R, VP_BG_G, VP_BG_B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    executorRun(g_script, vpW, vpH, s_camRotX, s_camRotY);

    // Label 2D sobreposto no viewport
    beginPanel(PALETTE_W + SCRIPT_W, FOOTER_H, vpW, vpH, true);
    {
        // Mostra o modo de câmera ativo
        const char* camMode = "Perspectiva";
        for (const Block& b : g_script) {
            if (b.enabled && b.type == BLK_CAM_ORTHO)    { camMode = "Ortografica"; break; }
            if (b.enabled && b.type == BLK_CAM_PERSPECTIVE) { camMode = "Perspectiva"; break; }
        }
        char camBuf[32];
        snprintf(camBuf, sizeof(camBuf), "Cam: %s", camMode);
        uiText(8.0f, 18.0f, camBuf, 0, 0.25f, 0.28f, 0.42f);
        uiText(8.0f, 32.0f, "[R] reset cam  [1-4] presets", 0,
               0.52f, 0.54f, 0.64f);
    }
    uiText((float)(vpW - 100), (float)(vpH - 20), "Viewport 3D", 0,
           0.48f, 0.50f, 0.62f);
    if (s_dragging)
        uiText(8.0f, 54.0f, "Arrastando camara...", 0, 0.25f, 0.28f, 0.42f);

    // --- Rodape de codigo (tema claro) ---
    beginPanel(0, 0, WIN_W, FOOTER_H, true);
    uiRect(0, 0, WIN_W, FOOTER_H, UI_BG_R - 0.02f, UI_BG_G - 0.02f, UI_BG_B);
    uiHLine(0, 0, WIN_W, SEP_R, SEP_G, SEP_B);
    uiText(12, 14, "CODIGO OPENGL GERADO:", 0, TXT2_R, TXT2_G, TXT2_B);
    uiButton((float)(WIN_W - 90), 4.f, 82.f, 22.f, "[ Copiar ]", 0.28f, 0.52f, 0.78f);
    uiHLine(0, 32, WIN_W, SEP_R, SEP_G, SEP_B);

    {
        std::string code = codegenRun(g_script);
        std::istringstream iss(code);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(iss, line))
            if (!line.empty()) lines.push_back(line);

        // Clampeia scroll
        int maxScroll = (int)lines.size() - 5;
        if (maxScroll < 0) maxScroll = 0;
        s_codeScrollMax = maxScroll;
        if (s_codeScroll > maxScroll) s_codeScroll = maxScroll;
        if (s_codeScroll < 0)         s_codeScroll = 0;

        int lineY = 46;
        for (int i = s_codeScroll; i < (int)lines.size() && i < s_codeScroll + 5; ++i) {
            uiText(12, (float)lineY, lines[i].c_str(), 0, 0.10f, 0.45f, 0.22f);
            lineY += 18;
        }

        // Scrollbar vertical à direita do rodapé
        if ((int)lines.size() > 5) {
            const float trackX = (float)(WIN_W - 12);
            const float trackW = 12.0f;
            const float trackY = 34.0f;
            const float trackH = (float)(FOOTER_H - 36);

            uiRect(trackX, trackY, trackW, trackH, 0.80f, 0.82f, 0.87f);

            float ratio  = 5.0f / (float)lines.size();
            float thumbH = trackH * ratio;
            if (thumbH < 16.0f) thumbH = 16.0f;
            float thumbY = trackY + (float)s_codeScroll * (trackH - thumbH) / (float)(maxScroll > 0 ? maxScroll : 1);
            uiRect(trackX, thumbY, trackW, thumbH, 0.52f, 0.55f, 0.68f);
        }
    }

    glDisable(GL_SCISSOR_TEST);
    glutSwapBuffers();
    g_needsRedraw = false;
}

// ---------------------------------------------------------------------------
// Callbacks de mouse
// ---------------------------------------------------------------------------
void mouse(int button, int state, int mx, int my)
{
    int vpH = WIN_H - FOOTER_H;
    int vpX = PALETTE_W + SCRIPT_W;  // início do viewport 3D em X

    // Scroll wheel (macOS GLUT: botões 3 e 4)
    if (state == GLUT_DOWN && (button == 3 || button == 4)) {
        int dir = (button == 3) ? 1 : -1;
        if (mx >= 0 && mx < PALETTE_W && my < vpH)
            paletteScroll(dir);
        else if (mx >= PALETTE_W && mx < PALETTE_W + SCRIPT_W && my < vpH)
            scriptScroll(dir);
        glutPostRedisplay();
        return;
    }

    // Drag na scrollbar do rodapé de código
    if (button == GLUT_LEFT_BUTTON && mx >= WIN_W - 12 && my >= vpH) {
        s_codeScrollDrag = (state == GLUT_DOWN);
        if (s_codeScrollDrag) {
            // Clique direto: mapeia posição Y para scroll
            float trackY0 = (float)(WIN_H - FOOTER_H + 34);
            float trackH  = (float)(FOOTER_H - 36);
            float t = ((float)my - trackY0) / trackH;
            if (t < 0.f) t = 0.f;
            if (t > 1.f) t = 1.f;
            s_codeScroll = (int)(t * s_codeScrollMax + 0.5f);
            glutPostRedisplay();
        }
        return;
    }

    // Botão Copiar no rodapé
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && my >= vpH) {
        int btnX0 = WIN_W - 90, btnX1 = WIN_W - 8;
        int btnY0 = WIN_H - FOOTER_H + 4, btnY1 = WIN_H - FOOTER_H + 26;
        if (mx >= btnX0 && mx <= btnX1 && my >= btnY0 && my <= btnY1) {
            copyCodeToClipboard();
            return;
        }
    }

    // Drag orbital no viewport 3D
    if (mx >= vpX && my < vpH) {
        if (button == GLUT_LEFT_BUTTON) {
            s_dragging  = (state == GLUT_DOWN);
            s_dragLastX = mx;
            s_dragLastY = my;
        }
        return;
    }

    if (button != GLUT_LEFT_BUTTON) return;

    // Clique na paleta — só no DOWN
    if (mx >= 0 && mx < PALETTE_W && my >= 0 && my < vpH) {
        if (state == GLUT_DOWN) {
            paletteMouseClick(mx, my);
            glutPostRedisplay();
        }
        return;
    }

    // Área do script — roteia DOWN/UP para o novo sistema de sliders
    if (mx >= PALETTE_W && mx < PALETTE_W + SCRIPT_W && my >= 0 && my < vpH) {
        int lx = mx - PALETTE_W;
        if (state == GLUT_DOWN) scriptMouseDown(lx, my);
        else                    scriptMouseUp(lx, my);
        glutPostRedisplay();
        return;
    }
}

void mouseMotion(int mx, int my)
{
    // Drag na scrollbar do rodapé de código
    if (s_codeScrollDrag) {
        float trackY0 = (float)(WIN_H - FOOTER_H + 34);
        float trackH  = (float)(FOOTER_H - 36);
        float t = ((float)my - trackY0) / trackH;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        s_codeScroll = (int)(t * s_codeScrollMax + 0.5f);
        glutPostRedisplay();
        return;
    }

    // Drag de slider no painel de script tem prioridade sobre câmera orbital
    if (scriptIsDragging()) {
        if (mx >= PALETTE_W && mx < PALETTE_W + SCRIPT_W)
            scriptMouseMotion(mx - PALETTE_W, my);
        glutPostRedisplay();
        return;
    }

    if (!s_dragging) return;
    float dx = (float)(mx - s_dragLastX);
    float dy = (float)(my - s_dragLastY);
    s_camRotY += dx * 0.5f;
    s_camRotX += dy * 0.5f;
    if (s_camRotX >  89.f) s_camRotX =  89.f;
    if (s_camRotX < -89.f) s_camRotX = -89.f;
    s_dragLastX = mx;
    s_dragLastY = my;
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 80);
    glutCreateWindow("OpenGL Scratch -- CG 2026.1");

    paletteInit();
    paletteSetAddCallback(addBlockToScript);
    scriptInit();
    executorInit();

    glutDisplayFunc(display);
    glutReshapeFunc([](int w, int h){ glViewport(0, 0, w, h); });
    glutKeyboardFunc([](unsigned char k, int, int){
        if (k == 27) exit(0);
        if (k == 'r' || k == 'R') {
            s_camRotX = 20.0f;
            s_camRotY = -30.0f;
            glutPostRedisplay();
        }
        if (k >= '1' && k <= '4') {
            loadPreset(k - '0');
        }
        if (k == ']') { s_codeScroll++; glutPostRedisplay(); }
        if (k == '[') { s_codeScroll--; glutPostRedisplay(); }
    });
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutSpecialFunc([](int key, int /*x*/, int /*y*/){
        if (key == GLUT_KEY_DOWN)       scriptScroll(-1);
        else if (key == GLUT_KEY_UP)    scriptScroll( 1);
        else if (key == GLUT_KEY_PAGE_DOWN) scriptScroll(-5);
        else if (key == GLUT_KEY_PAGE_UP)   scriptScroll( 5);
        glutPostRedisplay();
    });

    glutMainLoop();
    return 0;
}
