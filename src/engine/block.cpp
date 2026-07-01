#include "block.h"
#include <cstring>   // memset, strncpy

// ============================================================
//  Tabela estática de definições de bloco
//  Ordem deve corresponder exatamente ao enum BlockType.
// ============================================================

// Macro auxiliar para ParamDef — reduz repetição
#define P(lbl, mn, mx, def) { (lbl), (mn), (mx), (def) }

// Entradas sem parâmetros visuais usam esta lista vazia
#define NO_PARAMS \
    { P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), \
      P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f) }

static const BlockDef g_blockDefs[BLK_COUNT] = {

    // -------------------------------------------------------
    //  OBJETOS  (categoria 0 — azul)
    //  cor: R=0.25  G=0.50  B=1.00
    // -------------------------------------------------------
    {
        BLK_CUBE, "Cubo", "Desenha um cubo 3D",
        0,  0.25f, 0.50f, 1.00f,
        1,
        {
            P("tamanho", 0.1f, 3.0f, 1.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_SPHERE, "Esfera", "Desenha uma esfera 3D",
        0,  0.25f, 0.50f, 1.00f,
        3,
        {
            P("raio",   0.1f,  3.0f,  0.7f),
            P("fatias", 4.0f, 32.0f, 16.0f),
            P("pilhas", 4.0f, 32.0f, 16.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_TEAPOT, "Bule", "Desenha o famoso bule 3D (Teapot)",
        0,  0.25f, 0.50f, 1.00f,
        1,
        {
            P("tamanho", 0.1f, 3.0f, 1.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_PLANE, "Plano", "Desenha um plano flat (chao)",
        0,  0.25f, 0.50f, 1.00f,
        2,
        {
            P("largura",     0.5f, 5.0f, 2.0f),
            P("profundidade", 0.5f, 5.0f, 2.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_CONE, "Cone", "Desenha um cone 3D",
        0,  0.25f, 0.50f, 1.00f,
        3,
        {
            P("base",   0.1f,  2.0f,  0.5f),
            P("altura", 0.1f,  3.0f,  1.0f),
            P("fatias", 4.0f, 32.0f, 16.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_CYLINDER, "Cilindro", "Desenha um cilindro 3D",
        0,  0.25f, 0.50f, 1.00f,
        3,
        {
            P("raio",   0.1f,  2.0f,  0.4f),
            P("altura", 0.1f,  3.0f,  1.0f),
            P("fatias", 4.0f, 32.0f, 16.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },

    // -------------------------------------------------------
    //  TRANSFORMAÇÕES  (categoria 1 — laranja)
    //  cor: R=1.00  G=0.60  B=0.20
    // -------------------------------------------------------
    {
        BLK_TRANSLATE, "Mover", "Move o objeto na cena (X, Y, Z)",
        1,  1.00f, 0.60f, 0.20f,
        3,
        {
            P("X", -5.0f, 5.0f, 0.0f),
            P("Y", -5.0f, 5.0f, 0.0f),
            P("Z", -5.0f, 5.0f, 0.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_ROTATE, "Girar", "Gira o objeto em torno de um eixo",
        1,  1.00f, 0.60f, 0.20f,
        4,
        {
            P("angulo", -360.0f, 360.0f, 45.0f),
            P("eixoX",    -1.0f,   1.0f,  0.0f),
            P("eixoY",    -1.0f,   1.0f,  1.0f),
            P("eixoZ",    -1.0f,   1.0f,  0.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_SCALE, "Escalar", "Muda o tamanho do objeto",
        1,  1.00f, 0.60f, 0.20f,
        3,
        {
            P("X", 0.1f, 5.0f, 1.0f),
            P("Y", 0.1f, 5.0f, 1.0f),
            P("Z", 0.1f, 5.0f, 1.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_PUSH_POP, "Isolar", "Isola as transformacoes dos proximos blocos",
        1,  1.00f, 0.60f, 0.20f,
        0,
        NO_PARAMS
    },

    // -------------------------------------------------------
    //  CÂMERA / PROJEÇÃO  (categoria 2 — roxo)
    //  cor: R=0.70  G=0.40  B=1.00
    // -------------------------------------------------------
    {
        BLK_CAM_PERSPECTIVE, "Camera Perspectiva",
        "Visao 3D com profundidade (como nossos olhos)",
        2,  0.70f, 0.40f, 1.00f,
        3,
        {
            P("campo_visao",  20.0f, 120.0f, 60.0f),
            P("perto",         0.01f,  1.0f,  0.1f),
            P("longe",         5.0f, 200.0f, 50.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_CAM_ORTHO, "Camera Ortografica",
        "Visao sem profundidade (como plantas e mapas)",
        2,  0.70f, 0.40f, 1.00f,
        1,
        {
            P("zoom", 0.5f, 10.0f, 3.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_CAM_LOOKAT, "Posicionar Camera",
        "Define de onde a camera ve a cena",
        2,  0.70f, 0.40f, 1.00f,
        6,
        {
            P("olho_X", -10.0f, 10.0f, 3.0f),
            P("olho_Y", -10.0f, 10.0f, 3.0f),
            P("olho_Z", -10.0f, 10.0f, 3.0f),
            P("alvo_X",  -5.0f,  5.0f, 0.0f),
            P("alvo_Y",  -5.0f,  5.0f, 0.0f),
            P("alvo_Z",  -5.0f,  5.0f, 0.0f)
        }
    },

    // -------------------------------------------------------
    //  ILUMINAÇÃO  (categoria 3 — amarelo)
    //  cor: R=1.00  G=0.85  B=0.20
    // -------------------------------------------------------
    {
        BLK_LIGHT_ENABLE, "Ligar Luz", "Ativa a iluminacao na cena",
        3,  1.00f, 0.85f, 0.20f,
        0,
        NO_PARAMS
    },
    {
        BLK_LIGHT_DIR, "Luz Direcional",
        "Luz que vem de uma direcao (como o sol)",
        3,  1.00f, 0.85f, 0.20f,
        4,
        {
            P("dir_X",      -1.0f, 1.0f, 0.5f),
            P("dir_Y",      -1.0f, 1.0f, 1.0f),
            P("dir_Z",      -1.0f, 1.0f, 0.5f),
            P("intensidade", 0.0f, 1.0f, 1.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_LIGHT_POINT, "Luz Pontual",
        "Luz que emana de um ponto (como lampada)",
        3,  1.00f, 0.85f, 0.20f,
        4,
        {
            P("pos_X",      -5.0f, 5.0f, 2.0f),
            P("pos_Y",      -5.0f, 5.0f, 3.0f),
            P("pos_Z",      -5.0f, 5.0f, 2.0f),
            P("intensidade", 0.0f, 1.0f, 1.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_MATERIAL, "Material",
        "Define cor e brilho da superficie",
        3,  1.00f, 0.85f, 0.20f,
        4,
        {
            P("R",      0.0f,   1.0f,  0.8f),
            P("G",      0.0f,   1.0f,  0.3f),
            P("B",      0.0f,   1.0f,  0.2f),
            P("brilho", 0.0f, 128.0f, 32.0f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },
    {
        BLK_AMBIENT_COLOR, "Cor Ambiente",
        "Cor da luz que ilumina tudo por igual",
        3,  1.00f, 0.85f, 0.20f,
        3,
        {
            P("R", 0.0f, 1.0f, 0.2f),
            P("G", 0.0f, 1.0f, 0.2f),
            P("B", 0.0f, 1.0f, 0.2f),
            P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f), P("", 0.f, 1.f, 0.f)
        }
    },

    // -------------------------------------------------------
    //  TEXTURA E CURVAS  (categoria 4 — verde)
    //  cor: R=0.30  G=0.85  B=0.50
    // -------------------------------------------------------
    {
        BLK_TEXTURE, "Textura",
        "Aplica uma imagem na superficie do objeto",
        4,  0.30f, 0.85f, 0.50f,
        0,
        NO_PARAMS
        // strParam é utilizado para o nome do arquivo de textura
    },
    {
        BLK_BEZIER, "Curva Bezier",
        "Desenha uma curva com pontos de controle",
        4,  0.30f, 0.85f, 0.50f,
        6,
        {
            P("p0x", -3.0f, 3.0f, -1.5f),
            P("p0y", -3.0f, 3.0f,  0.0f),
            P("p1x", -3.0f, 3.0f, -0.5f),
            P("p1y", -3.0f, 3.0f,  1.5f),
            P("p2x", -3.0f, 3.0f,  0.5f),
            P("p2y", -3.0f, 3.0f,  1.5f)
            // p3 implícito: (1.5, 0)
        }
    }
};

#undef P
#undef NO_PARAMS

// ============================================================
//  getBlockDef
// ============================================================
const BlockDef& getBlockDef(BlockType type)
{
    // Fallback seguro: retorna BLK_CUBE para tipos fora do intervalo
    if (type < 0 || type >= BLK_COUNT)
        return g_blockDefs[BLK_CUBE];
    return g_blockDefs[type];
}

// ============================================================
//  makeBlock
// ============================================================
Block makeBlock(BlockType type)
{
    Block b;
    b.type    = type;
    b.enabled = true;
    memset(b.params,   0, sizeof(b.params));
    memset(b.strParam, 0, sizeof(b.strParam));

    const BlockDef &def = getBlockDef(type);
    for (int i = 0; i < def.paramCount && i < 6; i++)
        b.params[i] = def.params[i].defaultVal;

    // strParam padrão para textura
    if (type == BLK_TEXTURE)
        strncpy(b.strParam, "madeira", sizeof(b.strParam) - 1);

    return b;
}

// ============================================================
//  getCategoryName
// ============================================================
const char* getCategoryName(int category)
{
    switch (category) {
        case 0: return "Objetos";
        case 1: return "Mover";
        case 2: return "Camera";
        case 3: return "Luz";
        case 4: return "Textura";
        default: return "Desconhecido";
    }
}

// ============================================================
//  getCategoryColor
// ============================================================
void getCategoryColor(int category, float &r, float &g, float &b)
{
    switch (category) {
        case 0:  r = 0.25f; g = 0.50f; b = 1.00f; break;  // azul
        case 1:  r = 1.00f; g = 0.60f; b = 0.20f; break;  // laranja
        case 2:  r = 0.70f; g = 0.40f; b = 1.00f; break;  // roxo
        case 3:  r = 1.00f; g = 0.85f; b = 0.20f; break;  // amarelo
        case 4:  r = 0.30f; g = 0.85f; b = 0.50f; break;  // verde
        default: r = 0.50f; g = 0.50f; b = 0.50f; break;  // cinza fallback
    }
}
