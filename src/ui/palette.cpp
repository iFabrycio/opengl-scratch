#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include "palette.h"
#include "panel.h"
#include "engine/block.h"
#include "config.h"

#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Estado interno
// ---------------------------------------------------------------------------
static void (*s_addCallback)(BlockType) = nullptr;
static bool  s_catExpanded[5] = { true, false, false, false, false };
static int   s_scrollY        = 0;   // pixels rolados (0 = topo)
static int   s_hoveredBlock   = -1;  // índice em BlockType, -1 = nenhum

// Structs auxiliares para hit-testing — reconstruídas a cada frame
struct HitItem { BlockType type; int y0, y1; };
static std::vector<HitItem> s_hitItems;

struct HitCat { int cat; int y0, y1; };
static std::vector<HitCat>  s_hitCats;

// ---------------------------------------------------------------------------
// Dimensões de layout
// ---------------------------------------------------------------------------
static const int CAT_HEADER_H = 26;   // altura do cabeçalho de categoria
static const int ITEM_H       = 22;   // altura de cada item de bloco
static const int TITLE_H      = 36;   // altura do bloco de título

// Letra do ícone de cada categoria
static char categoryLetter(int cat)
{
    switch (cat) {
        case 0: return 'O';
        case 1: return 'M';
        case 2: return 'C';
        case 3: return 'L';
        case 4: return 'T';
        default: return '?';
    }
}

// ---------------------------------------------------------------------------
// paletteInit
// ---------------------------------------------------------------------------
void paletteInit()
{
    s_catExpanded[0] = true;
    s_catExpanded[1] = false;
    s_catExpanded[2] = false;
    s_catExpanded[3] = false;
    s_catExpanded[4] = false;
    s_scrollY    = 0;
    s_hoveredBlock = -1;
    s_hitItems.clear();
    s_hitCats.clear();
}

// ---------------------------------------------------------------------------
// paletteSetAddCallback
// ---------------------------------------------------------------------------
void paletteSetAddCallback(void (*cb)(BlockType))
{
    s_addCallback = cb;
}

// ---------------------------------------------------------------------------
// paletteScroll
// ---------------------------------------------------------------------------
void paletteScroll(int delta)
{
    s_scrollY -= delta * 20;
    if (s_scrollY < 0) s_scrollY = 0;
    // Limite superior generoso; paletteDraw clampará caso precise
    if (s_scrollY > 800) s_scrollY = 800;
}

// ---------------------------------------------------------------------------
// paletteDraw
// ---------------------------------------------------------------------------
void paletteDraw(int panelW, int panelH)
{
    // Fundo do painel
    uiRect(0, 0, (float)panelW, (float)panelH,
           UI_BG_R, UI_BG_G, UI_BG_B + 0.03f);

    // Título fixo (não rola)
    uiText(12, 8, "BLOCOS", 1, TXT_R, TXT_G, TXT_B);
    uiHLine(0, (float)TITLE_H, (float)panelW, SEP_R, SEP_G, SEP_B);

    // Reconstrução das listas de hit-test
    s_hitItems.clear();
    s_hitCats.clear();

    // Calcula conteúdo total para a barra de scroll
    int totalContent = 0;
    for (int cat = 0; cat < 5; ++cat) {
        totalContent += CAT_HEADER_H;
        if (s_catExpanded[cat]) {
            for (int t = 0; t < (int)BLK_COUNT; ++t) {
                if (getBlockDef((BlockType)t).category == cat)
                    totalContent += ITEM_H;
            }
        }
    }

    // Clampeia scroll
    int maxScroll = totalContent - (panelH - TITLE_H);
    if (maxScroll < 0) maxScroll = 0;
    if (s_scrollY > maxScroll) s_scrollY = maxScroll;

    // Cursor Y de desenho (começa após o título, deslocado pelo scroll)
    float curY = (float)TITLE_H - (float)s_scrollY;

    // Habilita scissor no espaço do painel para cortar conteúdo acima do título
    // (o scissor geral já está ligado pelo caller; aqui apenas controlamos
    //  não desenhar fora da área visível via checagem de curY)

    for (int cat = 0; cat < 5; ++cat) {
        float r, g, b;
        getCategoryColor(cat, r, g, b);

        // --- Cabeçalho de categoria ---
        float hdrY = curY;
        float hdrH = (float)CAT_HEADER_H;

        // Registra hit-test mesmo se parcialmente fora da tela
        {
            HitCat hc;
            hc.cat = cat;
            hc.y0  = (int)hdrY;
            hc.y1  = (int)(hdrY + hdrH);
            s_hitCats.push_back(hc);
        }

        // Só desenha se visível
        if (hdrY + hdrH > (float)TITLE_H && hdrY < (float)panelH) {
            // Fundo sólido na cor da categoria (vibrante, como Scratch)
            uiRect(0, hdrY, (float)panelW, hdrH,
                   r * 0.78f, g * 0.78f, b * 0.78f);

            // Ícone branco à direita
            uiCategoryIcon(178.0f, hdrY + hdrH * 0.5f, 9.0f,
                           1.0f, 1.0f, 1.0f, categoryLetter(cat));

            // Seta + nome com texto branco
            std::string arrow = s_catExpanded[cat] ? "v " : "> ";
            std::string label = arrow + getCategoryName(cat);
            uiText(8.0f, hdrY + 5.0f, label, 0, 0.98f, 0.98f, 0.98f);
        }

        curY += hdrH;

        // --- Itens do bloco (só se expandido) ---
        if (s_catExpanded[cat]) {
            for (int t = 0; t < (int)BLK_COUNT; ++t) {
                const BlockDef& def = getBlockDef((BlockType)t);
                if (def.category != cat) continue;

                float itemY = curY;
                float itemH = (float)ITEM_H;

                // Hit-test
                {
                    HitItem hi;
                    hi.type = (BlockType)t;
                    hi.y0   = (int)itemY;
                    hi.y1   = (int)(itemY + itemH);
                    s_hitItems.push_back(hi);
                }

                // Só desenha se visível
                if (itemY + itemH > (float)TITLE_H && itemY < (float)panelH) {
                    bool hovered = (s_hoveredBlock == t);
                    // Fundo: tint pastel suave da categoria
                    float tint = hovered ? 0.14f : 0.07f;
                    float base = hovered ? 0.84f : 0.91f;
                    uiRect(0, itemY, (float)panelW, itemH,
                           base + r*tint, base + g*tint, base + b*tint);

                    // Bollinha colorida à esquerda
                    uiCategoryIcon(10.0f, itemY + itemH * 0.5f, 4.5f,
                                   r, g, b, ' ');

                    // Nome do bloco
                    uiText(20.0f, itemY + 4.0f, def.name, 0,
                           TXT_R, TXT_G, TXT_B);
                }

                curY += itemH;
            }
        }

        // Separador após cada categoria
        if (curY > (float)TITLE_H && curY <= (float)panelH) {
            uiHLine(0, curY, (float)panelW, SEP_R, SEP_G, SEP_B);
        }
    }

    // --- Barra de scroll (se necessário) ---
    if (totalContent > panelH - TITLE_H) {
        const float trackX = (float)(panelW - 4);
        const float trackW = 4.0f;
        const float trackY = (float)TITLE_H;
        const float trackH = (float)(panelH - TITLE_H);

        // Track claro
        uiRect(trackX, trackY, trackW, trackH,
               0.80f, 0.82f, 0.87f);

        // Thumb
        float ratio     = trackH / (float)totalContent;
        float thumbH    = trackH * ratio;
        if (thumbH < 16.0f) thumbH = 16.0f;
        float thumbYOff = (float)s_scrollY * (trackH - thumbH) / (float)(maxScroll > 0 ? maxScroll : 1);
        uiRect(trackX, trackY + thumbYOff, trackW, thumbH,
               0.52f, 0.55f, 0.68f);
    }

    // Separador vertical direito
    uiRect((float)(panelW - 2), 0, 2.0f, (float)panelH, SEP_R, SEP_G, SEP_B);
}

// ---------------------------------------------------------------------------
// paletteMouseClick
// ---------------------------------------------------------------------------
void paletteMouseClick(int mx, int my)
{
    (void)mx;

    // 1. Testa clique em cabeçalho de categoria → toggle expand
    for (size_t i = 0; i < s_hitCats.size(); ++i) {
        const HitCat& hc = s_hitCats[i];
        if (my >= hc.y0 && my < hc.y1) {
            s_catExpanded[hc.cat] = !s_catExpanded[hc.cat];
            return;
        }
    }

    // 2. Testa clique em item de bloco → dispara callback
    for (size_t i = 0; i < s_hitItems.size(); ++i) {
        const HitItem& hi = s_hitItems[i];
        if (my >= hi.y0 && my < hi.y1) {
            if (s_addCallback) s_addCallback(hi.type);
            return;
        }
    }
}
