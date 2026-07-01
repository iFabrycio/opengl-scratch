#pragma once
#include "engine/block.h"  // BlockType

// Inicializa estado da paleta (chame uma vez no main)
void paletteInit();

// Renderiza o painel da paleta inteiro.
// Deve ser chamado após beginPanel(0, FOOTER_H, PALETTE_W, vpH, true)
// panelW e panelH são as dimensões do viewport corrente.
void paletteDraw(int panelW, int panelH);

// Processa clique do mouse em coords do painel da paleta (Y=0 no topo).
// mx, my já convertidos para coords locais do painel.
void paletteMouseClick(int mx, int my);

// Scroll da paleta (delta: +1 = rola para cima, -1 = para baixo)
void paletteScroll(int delta);

// Registra callback chamado quando o usuário clica em um bloco.
// O callback recebe o BlockType a ser adicionado ao script.
void paletteSetAddCallback(void (*cb)(BlockType));
