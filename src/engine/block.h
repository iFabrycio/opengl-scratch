#ifndef BLOCK_H
#define BLOCK_H

// ============================================================
//  block.h  –  Modelo de dados central dos blocos
//  Sem dependências de OpenGL/GLUT — só define estruturas e
//  metadados estáticos dos tipos de bloco disponíveis.
// ============================================================

// -----------------------------------------------------------
//  Tipos de bloco
// -----------------------------------------------------------
enum BlockType {
    // Objetos (azul — categoria 0)
    BLK_CUBE = 0,
    BLK_SPHERE,
    BLK_TEAPOT,
    BLK_PLANE,
    BLK_CONE,
    BLK_CYLINDER,

    // Transformações (laranja — categoria 1)
    BLK_TRANSLATE,
    BLK_ROTATE,
    BLK_SCALE,
    BLK_PUSH_POP,   // aplica glPushMatrix/glPopMatrix em volta dos próximos N blocos

    // Câmera/Projeção (roxo — categoria 2)
    BLK_CAM_PERSPECTIVE,
    BLK_CAM_ORTHO,
    BLK_CAM_LOOKAT,

    // Iluminação (amarelo — categoria 3)
    BLK_LIGHT_ENABLE,
    BLK_LIGHT_DIR,
    BLK_LIGHT_POINT,
    BLK_MATERIAL,
    BLK_AMBIENT_COLOR,

    // Textura e Curvas (verde — categoria 4)
    BLK_TEXTURE,
    BLK_BEZIER,

    BLK_COUNT   // sentinela — total de tipos
};

// -----------------------------------------------------------
//  Instância de um bloco na cena
// -----------------------------------------------------------
struct Block {
    BlockType type;
    float     params[12];   // parâmetros numéricos (ver BlockDef para significado)
    char      strParam[64]; // parâmetro string (ex: nome de textura)
    bool      enabled;      // se false, o executor ignora este bloco
};

// -----------------------------------------------------------
//  Metadados estáticos de cada tipo de bloco
// -----------------------------------------------------------
struct ParamDef {
    const char *label;      // nome amigável exibido no slider
    float       minVal;
    float       maxVal;
    float       defaultVal;
};

struct BlockDef {
    BlockType   type;
    const char *name;       // nome exibido no bloco
    const char *tooltip;    // explicação simples para crianças
    int         category;   // 0=Objetos, 1=Mover, 2=Camera, 3=Luz, 4=Textura
    float       colorR, colorG, colorB;  // cor do bloco na UI
    int         paramCount; // quantos params[i] este bloco usa (máx. 6 visíveis)
    ParamDef    params[6];  // metadados dos parâmetros visíveis
    // Nota: Block.params[12] acomoda até 12 floats, mas exibimos no máximo 6 sliders
};

// -----------------------------------------------------------
//  Funções públicas
// -----------------------------------------------------------

// Retorna a definição estática de um tipo de bloco.
// Se type for inválido retorna a definição de BLK_CUBE como fallback seguro.
const BlockDef& getBlockDef(BlockType type);

// Cria um bloco com valores padrão definidos em BlockDef
Block makeBlock(BlockType type);

// Retorna nome textual da categoria
const char* getCategoryName(int category);

// Retorna cor da categoria (para ícones na UI)
void getCategoryColor(int category, float &r, float &g, float &b);

#endif // BLOCK_H
