#pragma once
#include <vector>
#include "engine/block.h"

// Script: lista ordenada de blocos montados pelo usuario
extern std::vector<Block> g_script;

// Indice do bloco selecionado no script (-1 = nenhum)
extern int g_selectedBlock;

// Flag: precisa re-renderizar
extern bool g_needsRedraw;
