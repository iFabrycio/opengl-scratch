#pragma once
#include <vector>
#include "block.h"

// Chama uma vez após glutCreateWindow (cria o GLUquadric)
void executorInit();

// Chama antes de fechar o programa
void executorCleanup();

// Executa o script no viewport 3D atual.
// vpW, vpH: dimensoes do viewport 3D em pixels (para aspect ratio da projeção).
// camRotX, camRotY: rotação orbital da câmera em graus (controlada pelo mouse em main.cpp).
void executorRun(const std::vector<Block>& script,
                 int vpW, int vpH,
                 float camRotX, float camRotY);
