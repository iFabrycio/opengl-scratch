#pragma once

void scriptInit();
void scriptDraw(int panelW, int panelH);

// Chamado no GLUT_DOWN em coords locais do painel (Y=0 no topo)
// Inicia drag de slider se o clique cair em um slider.
void scriptMouseDown(int mx, int my);

// Chamado pelo glutMotionFunc quando mouse se move com botão pressionado
// em coords locais do painel. Atualiza o param se slider ativo.
void scriptMouseMotion(int mx, int my);

// Chamado no GLUT_UP. Se nenhum slider foi arrastado, dispara lógica de
// clique (reordenar, remover, selecionar). Se arrastou, apenas limpa estado.
void scriptMouseUp(int mx, int my);

// Retorna true enquanto um slider está sendo arrastado.
// Usado em main.cpp para bloquear a câmera orbital durante drag de slider.
bool scriptIsDragging();

void scriptScroll(int delta);
