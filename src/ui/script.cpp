#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include "script.h"
#include "panel.h"
#include "config.h"
#include "state.h"
#include "engine/block.h"

#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>  // std::swap, std::max

// ---------------------------------------------------------------------------
// Constantes de layout
// ---------------------------------------------------------------------------
static const int HDR_H      = 36;  // altura do cabeçalho de cada bloco (nome + botões)
static const int PARAM_ROW  = 24;  // altura de cada linha de parâmetro
static const int BLOCK_PAD  = 10;  // espaçamento inferior de cada bloco
static const int TITLE_H    = 44;  // cabeçalho do painel (título + separador)
static const int FOOTER_BTN = 44;  // área do botão Executar no rodapé

// ---------------------------------------------------------------------------
// Função auxiliar blockHeight
// ---------------------------------------------------------------------------
static int blockHeight(const Block& b)
{
    const BlockDef& def = getBlockDef(b.type);
    if (def.paramCount == 0) return HDR_H + BLOCK_PAD;
    return HDR_H + def.paramCount * PARAM_ROW + BLOCK_PAD;
}

// ---------------------------------------------------------------------------
// Estado interno
// ---------------------------------------------------------------------------
static int  s_scrollY      = 0;
static int  s_scrollMax    = 0;   // atualizado em scriptDraw
static bool s_scrollDrag   = false;
static int  s_panelH       = 0;   // altura do painel, atualizado em scriptDraw

// Drag de slider
static bool  s_sliderDrag  = false;
static int   s_slDragBlock = -1;   // índice em g_script
static int   s_slDragParam = -1;   // índice do param
static float s_slTrackX0   = 0.f;
static float s_slTrackW    = 0.f;
static float s_slMinVal    = 0.f;
static float s_slMaxVal    = 1.f;

// Flag: o mouse desceu mas ainda não subiu (para distinguir click de drag)
static bool s_mouseDownPending = false;

// Posição Y do botão Executar (atualizada em scriptDraw)
static int s_executeY0 = 0;
static int s_executeY1 = 0;

// ---------------------------------------------------------------------------
// Estruturas de hit-testing
// ---------------------------------------------------------------------------
struct BlockHit {
    int idx;
    int y0, y1;     // limites do bloco inteiro
    int hdrY1;      // limite inferior do cabeçalho (para separar header de sliders)
};

struct SliderHit {
    int   blockIdx, paramIdx;
    float trackX0, trackW;   // posição e largura do track do slider em coords do painel
    int   y0, y1;            // limites verticais da linha do slider
    float minVal, maxVal;
};

static std::vector<BlockHit>  s_blockHits;
static std::vector<SliderHit> s_sliderHits;

// ---------------------------------------------------------------------------
// scriptInit
// ---------------------------------------------------------------------------
void scriptInit()
{
    s_scrollY = 0;
    s_sliderDrag = false;
    s_slDragBlock = -1;
    s_slDragParam = -1;
    s_mouseDownPending = false;
    s_executeY0 = 0;
    s_executeY1 = 0;
    s_blockHits.clear();
    s_sliderHits.clear();
}

// ---------------------------------------------------------------------------
// scriptScroll
// ---------------------------------------------------------------------------
void scriptScroll(int delta)
{
    s_scrollY -= delta * 20;
    if (s_scrollY < 0) s_scrollY = 0;
}

// ---------------------------------------------------------------------------
// scriptIsDragging
// ---------------------------------------------------------------------------
bool scriptIsDragging()
{
    return s_sliderDrag || s_scrollDrag;
}

// ---------------------------------------------------------------------------
// scriptDraw
// ---------------------------------------------------------------------------
void scriptDraw(int panelW, int panelH)
{
    s_panelH = panelH;

    // 1. Fundo do painel
    uiRect(0.f, 0.f, (float)panelW, (float)panelH,
           UI_BG_R, UI_BG_G, UI_BG_B);

    // 2. Cabeçalho fixo
    uiText(12.f, 8.f, "MEU SCRIPT", 1, TXT_R, TXT_G, TXT_B);
    uiHLine(0.f, 36.f, (float)panelW, SEP_R, SEP_G, SEP_B);

    // 3. Calcula altura total do conteúdo para clamp do scroll
    int totalH = 0;
    for (int i = 0; i < (int)g_script.size(); ++i)
        totalH += blockHeight(g_script[(size_t)i]);
    int visH = panelH - TITLE_H - FOOTER_BTN;
    if (visH < 0) visH = 0;
    s_scrollMax = std::max(0, totalH - visH);
    if (s_scrollY > s_scrollMax) s_scrollY = s_scrollMax;

    // 4. Reconstrói listas de hit-testing
    s_blockHits.clear();
    s_sliderHits.clear();

    // 5. Loop pelos blocos
    float curY = (float)TITLE_H - (float)s_scrollY;

    for (int i = 0; i < (int)g_script.size(); ++i) {
        const Block&    b   = g_script[(size_t)i];
        const BlockDef& def = getBlockDef(b.type);

        float bh = (float)blockHeight(b);
        float cr = def.colorR;
        float cg = def.colorG;
        float cb = def.colorB;

        // Visibilidade
        bool visible = (curY + bh > (float)TITLE_H) &&
                       (curY < (float)(panelH - FOOTER_BTN));

        if (visible) {
            // a) Fundo arredondado do bloco — pastel tintado da cor da categoria
            bool sel = (i == g_selectedBlock);
            float tint = sel ? 0.22f : 0.11f;
            float base = sel ? 0.76f : 0.87f;
            uiRoundRect(4.f, curY + 3.f, (float)(panelW - 8), bh - 6.f,
                        base + cr*tint, base + cg*tint, base + cb*tint, 10.f);
            // Barra de cor sólida no topo (4px, accent da categoria)
            uiRect(4.f, curY + 3.f, (float)(panelW - 8), 4.f, cr, cg, cb);

            // b) Header do bloco
            // Bollinha colorida
            uiCategoryIcon(14.f, curY + (float)HDR_H * 0.5f, 5.f, cr, cg, cb, ' ');
            // Nome
            uiText(24.f, curY + 8.f, def.name, 0, TXT_R, TXT_G, TXT_B);
            // Para BLK_TEXTURE, mostra nome da textura ativa e dica de clique
            if (b.type == BLK_TEXTURE) {
                uiText(24.f, curY + 20.f, b.strParam, 0, TXT2_R, TXT2_G, TXT2_B);
            }

            // Botões ▲▼✕ à direita
            float xX   = (float)panelW - 22.f;
            float xDn  = xX - 19.f;
            float xUp  = xDn - 19.f;
            float btnY = curY + ((float)HDR_H - 16.f) * 0.5f;
            uiIconButton(xUp, btnY, 16.f, 16.f, 'U', 0.80f, 0.83f, 0.94f);
            uiIconButton(xDn, btnY, 16.f, 16.f, 'D', 0.80f, 0.83f, 0.94f);
            uiIconButton(xX,  btnY, 16.f, 16.f, 'X', 0.95f, 0.72f, 0.72f);

            // c) Sliders (um por parâmetro, abaixo do header)
            for (int p = 0; p < def.paramCount; ++p) {
                float rowY   = curY + (float)HDR_H + (float)p * (float)PARAM_ROW;
                float labelW = 32.f;
                float valW   = 38.f;
                float trackX = labelW + 6.f;
                float trackW = (float)panelW - 16.f - trackX - valW;

                // Label do param
                uiText(8.f, rowY + 3.f, def.params[p].label, 0,
                       TXT2_R, TXT2_G, TXT2_B);

                // Slider
                uiSlider(trackX, rowY + 3.f, trackW,
                         b.params[p],
                         def.params[p].minVal, def.params[p].maxVal, "");

                // Valor numérico
                char valbuf[32];
                snprintf(valbuf, sizeof(valbuf), "%.2f", b.params[p]);
                uiText(trackX + trackW + 4.f, rowY + 3.f, valbuf, 0,
                       TXT_R, TXT_G, TXT_B);

                // Adiciona à s_sliderHits
                SliderHit sh;
                sh.blockIdx = i;
                sh.paramIdx = p;
                sh.trackX0  = trackX;
                sh.trackW   = trackW;
                sh.y0       = (int)(rowY);
                sh.y1       = (int)(rowY + (float)PARAM_ROW);
                sh.minVal   = def.params[p].minVal;
                sh.maxVal   = def.params[p].maxVal;
                s_sliderHits.push_back(sh);
            }
        } else {
            // Fora da área visível: ainda registra sliders para hit-testing
            for (int p = 0; p < def.paramCount; ++p) {
                float rowY   = curY + (float)HDR_H + (float)p * (float)PARAM_ROW;
                float labelW = 32.f;
                float valW   = 38.f;
                float trackX = labelW + 6.f;
                float trackW = (float)panelW - 16.f - trackX - valW;

                SliderHit sh;
                sh.blockIdx = i;
                sh.paramIdx = p;
                sh.trackX0  = trackX;
                sh.trackW   = trackW;
                sh.y0       = (int)(rowY);
                sh.y1       = (int)(rowY + (float)PARAM_ROW);
                sh.minVal   = def.params[p].minVal;
                sh.maxVal   = def.params[p].maxVal;
                s_sliderHits.push_back(sh);
            }
        }

        // d) Adiciona à s_blockHits
        BlockHit bh_hit;
        bh_hit.idx   = i;
        bh_hit.y0    = (int)curY;
        bh_hit.y1    = (int)(curY + bh);
        bh_hit.hdrY1 = (int)(curY + (float)HDR_H);
        s_blockHits.push_back(bh_hit);

        curY += bh;
    }

    // 6. Rodapé fixo
    float sepY = (float)(panelH - FOOTER_BTN);
    uiHLine(0.f, sepY, (float)panelW, SEP_R, SEP_G, SEP_B);

    float btnExY = sepY + 6.f;
    float btnExH = 28.f;
    s_executeY0 = (int)btnExY;
    s_executeY1 = (int)(btnExY + btnExH);

    if (g_script.empty()) {
        // Área visual de "arraste para cá" — borda tracejada centralizada
        float bx = 14.f, bw = (float)panelW - 28.f;
        float by = (float)TITLE_H + 20.f, bh = sepY - by - 20.f;
        if (bh > 20.f) {
            uiDashedHLine(bx,      by,      bw,    0.65f, 0.68f, 0.78f);
            uiDashedHLine(bx,      by + bh, bw,    0.65f, 0.68f, 0.78f);
            uiDashedVLine(bx,      by,      bh,    0.65f, 0.68f, 0.78f);
            uiDashedVLine(bx + bw, by,      bh,    0.65f, 0.68f, 0.78f);
        }
        // Seta apontando para esquerda (← paleta) e texto explicativo
        float mx2 = (float)panelW * 0.5f;
        float my2 = by + bh * 0.42f;
        // Seta ← desenhada com GL
        glColor3f(0.55f, 0.58f, 0.72f);
        glLineWidth(2.f);
        glBegin(GL_LINES);
            glVertex2f(mx2 + 22.f, my2);
            glVertex2f(mx2 - 22.f, my2);
        glEnd();
        glBegin(GL_TRIANGLES);
            glVertex2f(mx2 - 28.f, my2);
            glVertex2f(mx2 - 16.f, my2 - 7.f);
            glVertex2f(mx2 - 16.f, my2 + 7.f);
        glEnd();
        glLineWidth(1.f);
        // Texto de ajuda
        std::string h1 = "Clique em um bloco";
        std::string h2 = "na paleta para adicionar";
        float tw1 = uiTextWidth(h1, 0);
        float tw2 = uiTextWidth(h2, 0);
        uiText(mx2 - tw1 * 0.5f, my2 + 20.f, h1,  0, 0.52f, 0.55f, 0.68f);
        uiText(mx2 - tw2 * 0.5f, my2 + 34.f, h2,  0, 0.52f, 0.55f, 0.68f);

        // Na área do botão, mostra hint de presets
        std::string hp = "Atalhos: 1 2 3 4 = Presets";
        float twp = uiTextWidth(hp, 0);
        uiText(((float)panelW - twp) * 0.5f, sepY + 12.f, hp, 0,
               0.50f, 0.52f, 0.65f);
    } else {
        uiButton(8.f, btnExY, (float)(panelW - 16), btnExH,
                 "> Executar", 0.22f, 0.68f, 0.30f);
    }

    // 7. Scrollbar vertical (se houver conteúdo além da área visível)
    if (s_scrollMax > 0) {
        const float trackX = (float)(panelW - 10);
        const float trackW = 8.0f;
        const float trackY = (float)TITLE_H;
        const float trackH = sepY - trackY;

        uiRect(trackX, trackY, trackW, trackH, 0.80f, 0.82f, 0.87f);

        float ratio  = (float)visH / (float)(totalH > 0 ? totalH : 1);
        float thumbH = trackH * ratio;
        if (thumbH < 16.f) thumbH = 16.f;
        float thumbY = trackY + (float)s_scrollY * (trackH - thumbH) / (float)s_scrollMax;
        uiRect(trackX, thumbY, trackW, thumbH, 0.52f, 0.55f, 0.68f);
    }

    // 8. Separador vertical direito
    uiRect((float)(panelW - 2), 0.f, 2.f, (float)panelH,
           SEP_R, SEP_G, SEP_B);
}

// ---------------------------------------------------------------------------
// scriptMouseDown
// ---------------------------------------------------------------------------
void scriptMouseDown(int mx, int my)
{
    s_mouseDownPending = true;
    s_sliderDrag  = false;
    s_scrollDrag  = false;

    // Testa se clicou na scrollbar (faixa direita do painel)
    if (mx >= SCRIPT_W - 10 && s_scrollMax > 0) {
        s_scrollDrag = true;
        float trackY = (float)TITLE_H;
        float trackH = (float)(s_executeY0 - TITLE_H);
        float t = ((float)my - trackY) / trackH;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        s_scrollY = (int)(t * s_scrollMax + 0.5f);
        g_needsRedraw = true;
        return;
    }

    // Testa se clicou em algum slider
    for (size_t i = 0; i < s_sliderHits.size(); ++i) {
        const SliderHit& sh = s_sliderHits[i];
        if (my >= sh.y0 && my < sh.y1) {
            // Verifica se mx está na área do track (com margem de 7px do knob)
            if ((float)mx >= sh.trackX0 - 7.f &&
                (float)mx <= sh.trackX0 + sh.trackW + 7.f) {
                s_sliderDrag  = true;
                s_slDragBlock = sh.blockIdx;
                s_slDragParam = sh.paramIdx;
                s_slTrackX0   = sh.trackX0;
                s_slTrackW    = sh.trackW;
                s_slMinVal    = sh.minVal;
                s_slMaxVal    = sh.maxVal;
                // Atualiza valor imediatamente ao clicar
                float t = ((float)mx - sh.trackX0) / sh.trackW;
                if (t < 0.f) t = 0.f;
                if (t > 1.f) t = 1.f;
                g_script[(size_t)sh.blockIdx].params[sh.paramIdx] =
                    sh.minVal + t * (sh.maxVal - sh.minVal);
                g_needsRedraw = true;
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// scriptMouseMotion
// ---------------------------------------------------------------------------
void scriptMouseMotion(int mx, int my)
{
    // Drag na scrollbar
    if (s_scrollDrag) {
        float trackY = (float)TITLE_H;
        float trackH = (float)(s_executeY0 - TITLE_H);
        float t = ((float)my - trackY) / trackH;
        if (t < 0.f) t = 0.f;
        if (t > 1.f) t = 1.f;
        s_scrollY = (int)(t * s_scrollMax + 0.5f);
        g_needsRedraw = true;
        return;
    }

    (void)my;
    if (!s_sliderDrag || s_slDragBlock < 0) return;
    float t = ((float)mx - s_slTrackX0) / s_slTrackW;
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    g_script[(size_t)s_slDragBlock].params[(size_t)s_slDragParam] =
        s_slMinVal + t * (s_slMaxVal - s_slMinVal);
    g_needsRedraw = true;
}

// ---------------------------------------------------------------------------
// scriptMouseUp
// ---------------------------------------------------------------------------
void scriptMouseUp(int mx, int my)
{
    if (s_scrollDrag) { s_scrollDrag = false; return; }

    bool wasDragging = s_sliderDrag;
    s_sliderDrag  = false;
    s_slDragBlock = -1;
    s_slDragParam = -1;

    if (wasDragging) return;  // foi drag: não dispara click
    if (!s_mouseDownPending) return;
    s_mouseDownPending = false;

    int panelW = SCRIPT_W;

    // Botão Executar
    if (!g_script.empty() && my >= s_executeY0 && my < s_executeY1) {
        g_needsRedraw = true;
        return;
    }

    // Botões e corpo dos blocos
    for (size_t i = 0; i < s_blockHits.size(); ++i) {
        const BlockHit& bh = s_blockHits[i];
        if (my < bh.y0 || my >= bh.y1) continue;

        int idx = bh.idx;

        // Só processa botões se clique foi no header
        if (my >= bh.hdrY1) {
            // Clicou na área de sliders mas não foi drag — ignora
            return;
        }

        // Recalcula posições dos botões
        float xX   = (float)panelW - 22.f;
        float xDn  = xX - 19.f;
        float xUp  = xDn - 19.f;
        float btnY = (float)bh.y0 + ((float)HDR_H - 16.f) * 0.5f;
        float fmx  = (float)mx;
        float fmy  = (float)my;

        // Botão remover ✕
        if (fmx >= xX && fmx < xX + 16.f &&
            fmy >= btnY && fmy < btnY + 16.f) {
            g_script.erase(g_script.begin() + idx);
            if (g_selectedBlock >= (int)g_script.size())
                g_selectedBlock = (int)g_script.size() - 1;
            g_needsRedraw = true;
            return;
        }

        // Botão descer ▼
        if (fmx >= xDn && fmx < xDn + 16.f &&
            fmy >= btnY && fmy < btnY + 16.f) {
            if (idx < (int)g_script.size() - 1) {
                std::swap(g_script[(size_t)idx], g_script[(size_t)idx + 1]);
                if (g_selectedBlock == idx)       g_selectedBlock = idx + 1;
                else if (g_selectedBlock == idx + 1) g_selectedBlock = idx;
            }
            g_needsRedraw = true;
            return;
        }

        // Botão subir ▲
        if (fmx >= xUp && fmx < xUp + 16.f &&
            fmy >= btnY && fmy < btnY + 16.f) {
            if (idx > 0) {
                std::swap(g_script[(size_t)idx], g_script[(size_t)idx - 1]);
                if (g_selectedBlock == idx)       g_selectedBlock = idx - 1;
                else if (g_selectedBlock == idx - 1) g_selectedBlock = idx;
            }
            g_needsRedraw = true;
            return;
        }

        // Para BLK_TEXTURE: cicla a textura ativa ao clicar no corpo
        if (g_script[(size_t)idx].type == BLK_TEXTURE) {
            static const char* const texCycle[] = {"madeira","tijolo","xadrez","metal"};
            int cur = 0;
            for (int t = 0; t < 4; ++t)
                if (strcmp(g_script[(size_t)idx].strParam, texCycle[t]) == 0) { cur = t; break; }
            strncpy(g_script[(size_t)idx].strParam, texCycle[(cur+1)%4],
                    sizeof(g_script[(size_t)idx].strParam)-1);
            g_needsRedraw = true;
            return;
        }

        // Corpo do bloco: seleciona / desseleciona
        g_selectedBlock = (g_selectedBlock == idx) ? -1 : idx;
        g_needsRedraw = true;
        return;
    }
}
