#include "panel.h"

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <cmath>
#include <string>

// ---------------------------------------------------------------------------
// uiRect — retângulo sólido com suporte a alpha
// ---------------------------------------------------------------------------
void uiRect(float x, float y, float w, float h,
            float r, float g, float b, float a)
{
    if (a < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();

    if (a < 1.0f)
        glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// uiRoundRect — retângulo com cantos arredondados via TRIANGLE_FAN
// ---------------------------------------------------------------------------
void uiRoundRect(float x, float y, float w, float h,
                 float r, float g, float b, float cornerR)
{
    const int segments = 8;  // por canto

    glColor3f(r, g, b);

    // Centro do retângulo (corpo interno)
    glBegin(GL_QUADS);
        // interior horizontal (sem cantos)
        glVertex2f(x + cornerR,     y);
        glVertex2f(x + w - cornerR, y);
        glVertex2f(x + w - cornerR, y + h);
        glVertex2f(x + cornerR,     y + h);

        // faixas laterais
        glVertex2f(x,              y + cornerR);
        glVertex2f(x + cornerR,    y + cornerR);
        glVertex2f(x + cornerR,    y + h - cornerR);
        glVertex2f(x,              y + h - cornerR);

        glVertex2f(x + w - cornerR, y + cornerR);
        glVertex2f(x + w,           y + cornerR);
        glVertex2f(x + w,           y + h - cornerR);
        glVertex2f(x + w - cornerR, y + h - cornerR);
    glEnd();

    // Os quatro cantos como TRIANGLE_FAN
    // Centros dos arcos
    float cx[4] = { x + cornerR,     x + w - cornerR, x + w - cornerR, x + cornerR     };
    float cy[4] = { y + cornerR,     y + cornerR,     y + h - cornerR, y + h - cornerR };
    float startAngle[4] = { 180.0f, 270.0f, 0.0f, 90.0f };

    for (int c = 0; c < 4; ++c) {
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(cx[c], cy[c]);
            for (int i = 0; i <= segments; ++i) {
                float angle = (startAngle[c] + 90.0f * i / segments) * (float)M_PI / 180.0f;
                glVertex2f(cx[c] + cornerR * cosf(angle),
                           cy[c] + cornerR * sinf(angle));
            }
        glEnd();
    }
}

// ---------------------------------------------------------------------------
// uiText — texto com fonte bitmap GLUT
// ---------------------------------------------------------------------------
void uiText(float x, float y, const std::string& s, int font,
            float r, float g, float b)
{
    void* glutFont = GLUT_BITMAP_HELVETICA_18;
    if (font == 0)      glutFont = GLUT_BITMAP_HELVETICA_12;
    else if (font == 2) glutFont = GLUT_BITMAP_TIMES_ROMAN_24;

    glColor3f(r, g, b);
    // Ajusta Y: GLUT bitmap baseline é na parte inferior do glifo
    // Como Y=0 é o topo, somamos uma estimativa da altura da fonte
    float baseline = y + 12.0f;
    if (font == 1) baseline = y + 14.0f;
    if (font == 2) baseline = y + 18.0f;

    glRasterPos2f(x, baseline);
    for (char c : s)
        glutBitmapCharacter(glutFont, c);
}

// ---------------------------------------------------------------------------
// uiTextWidth — largura aproximada em pixels
// ---------------------------------------------------------------------------
float uiTextWidth(const std::string& s, int font)
{
    void* glutFont = GLUT_BITMAP_HELVETICA_18;
    if (font == 0)      glutFont = GLUT_BITMAP_HELVETICA_12;
    else if (font == 2) glutFont = GLUT_BITMAP_TIMES_ROMAN_24;

    int w = 0;
    for (char c : s)
        w += glutBitmapWidth(glutFont, c);
    return (float)w;
}

// ---------------------------------------------------------------------------
// uiHLine — linha horizontal separadora
// ---------------------------------------------------------------------------
void uiHLine(float x, float y, float w,
             float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_LINES);
        glVertex2f(x,     y + 0.5f);
        glVertex2f(x + w, y + 0.5f);
    glEnd();
}

// ---------------------------------------------------------------------------
// uiButton — botão com label centralizado; retorna true se hover (não clique real)
// Para clique real, o caller verifica coordenadas em callbacks de mouse.
// ---------------------------------------------------------------------------
bool uiButton(float x, float y, float w, float h,
              const std::string& label,
              float br, float bg, float bb,
              bool pressed)
{
    float factor = pressed ? 0.82f : 1.0f;
    uiRoundRect(x, y, w, h, br * factor, bg * factor, bb * factor, 6.0f);

    // Borda sutil (mais escura que o botão)
    glColor3f(br * 0.72f, bg * 0.72f, bb * 0.72f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();

    // Cor do texto: escuro em botões claros, claro em botões escuros (luminância)
    float lum = br * 0.299f + bg * 0.587f + bb * 0.114f;
    float tr, tg, tb;
    if (lum > 0.50f) { tr = 0.14f; tg = 0.15f; tb = 0.20f; }
    else             { tr = 0.95f; tg = 0.95f; tb = 0.98f; }

    // Texto centralizado
    float tw = uiTextWidth(label, 0);
    float tx = x + (w - tw) * 0.5f;
    float ty = y + (h - 12.0f) * 0.5f;
    uiText(tx, ty, label, 0, tr, tg, tb);

    return false;
}

// ---------------------------------------------------------------------------
// uiSlider — slider horizontal
// ---------------------------------------------------------------------------
void uiSlider(float x, float y, float w,
              float value, float minV, float maxV,
              const std::string& label)
{
    const float trackH = 4.0f;
    const float knobR  = 7.0f;
    const float trackY = y + knobR - trackH * 0.5f;

    // Track de fundo (cinza claro)
    uiRect(x, trackY, w, trackH, 0.78f, 0.80f, 0.86f);

    // Track preenchido (progresso — azul vibrante)
    float t = (maxV > minV) ? (value - minV) / (maxV - minV) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    uiRect(x, trackY, w * t, trackH, 0.28f, 0.55f, 0.95f);

    // Knob branco com anel
    float kx = x + w * t;
    float ky = y + knobR;
    // Sombra/anel externo
    glColor3f(0.55f, 0.60f, 0.75f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(kx, ky);
        for (int i = 0; i <= 16; ++i) {
            float angle = 2.0f * (float)M_PI * i / 16;
            glVertex2f(kx + knobR * cosf(angle), ky + knobR * sinf(angle));
        }
    glEnd();
    // Círculo branco interno
    glColor3f(0.99f, 0.99f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(kx, ky);
        for (int i = 0; i <= 16; ++i) {
            float angle = 2.0f * (float)M_PI * i / 16;
            glVertex2f(kx + (knobR-2.f) * cosf(angle), ky + (knobR-2.f) * sinf(angle));
        }
    glEnd();

    // Label à direita
    if (!label.empty()) {
        uiText(x + w + 8.0f, y, label, 0, 0.60f, 0.62f, 0.68f);
    }
}

// ---------------------------------------------------------------------------
// uiCategoryIcon — círculo colorido com letra
// ---------------------------------------------------------------------------
void uiCategoryIcon(float cx, float cy, float radius,
                    float r, float g, float b, char letter)
{
    const int segments = 24;

    // Círculo preenchido
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * (float)M_PI * i / segments;
            glVertex2f(cx + radius * cosf(angle), cy + radius * sinf(angle));
        }
    glEnd();

    // Letra centralizada
    char str[2] = { letter, '\0' };
    float tw = uiTextWidth(str, 0);
    uiText(cx - tw * 0.5f, cy - 6.0f, str, 0, 0.05f, 0.05f, 0.05f);
}

// ---------------------------------------------------------------------------
// uiIconButton — botão com ícone geométrico (sem texto bitmap)
// ---------------------------------------------------------------------------
void uiIconButton(float x, float y, float w, float h, char icon,
                  float br, float bg, float bb)
{
    uiRoundRect(x, y, w, h, br, bg, bb, 5.f);

    // Borda sutil
    glColor3f(br * 0.68f, bg * 0.68f, bb * 0.68f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x,     y + h);
    glEnd();

    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float s  = w * 0.27f;

    // Cor do ícone: escuro em fundo claro, claro em fundo escuro
    float lum = br * 0.299f + bg * 0.587f + bb * 0.114f;
    if (lum > 0.50f) glColor3f(0.14f, 0.16f, 0.26f);
    else             glColor3f(0.95f, 0.95f, 0.98f);

    if (icon == 'U') {
        // Triângulo apontando para cima
        glBegin(GL_TRIANGLES);
            glVertex2f(cx,         cy - s * 0.85f);
            glVertex2f(cx - s,     cy + s * 0.65f);
            glVertex2f(cx + s,     cy + s * 0.65f);
        glEnd();
    } else if (icon == 'D') {
        // Triângulo apontando para baixo
        glBegin(GL_TRIANGLES);
            glVertex2f(cx,         cy + s * 0.85f);
            glVertex2f(cx - s,     cy - s * 0.65f);
            glVertex2f(cx + s,     cy - s * 0.65f);
        glEnd();
    } else if (icon == 'X') {
        // Cruz diagonal ×
        float d = s * 0.75f;
        glLineWidth(2.2f);
        glBegin(GL_LINES);
            glVertex2f(cx - d, cy - d);  glVertex2f(cx + d, cy + d);
            glVertex2f(cx + d, cy - d);  glVertex2f(cx - d, cy + d);
        glEnd();
        glLineWidth(1.f);
    }
}

// ---------------------------------------------------------------------------
// uiDashedHLine / uiDashedVLine — linhas tracejadas para guias
// ---------------------------------------------------------------------------
void uiDashedHLine(float x, float y, float w,
                   float r, float g, float b,
                   float dashLen, float gapLen)
{
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    float period = dashLen + gapLen;
    for (float cx = x; cx < x + w; cx += period) {
        float x1 = cx;
        float x2 = cx + dashLen;
        if (x2 > x + w) x2 = x + w;
        glVertex2f(x1, y + 0.5f);
        glVertex2f(x2, y + 0.5f);
    }
    glEnd();
}

void uiDashedVLine(float x, float y, float h,
                   float r, float g, float b,
                   float dashLen, float gapLen)
{
    glColor3f(r, g, b);
    glBegin(GL_LINES);
    float period = dashLen + gapLen;
    for (float cy = y; cy < y + h; cy += period) {
        float y1 = cy;
        float y2 = cy + dashLen;
        if (y2 > y + h) y2 = y + h;
        glVertex2f(x + 0.5f, y1);
        glVertex2f(x + 0.5f, y2);
    }
    glEnd();
}
