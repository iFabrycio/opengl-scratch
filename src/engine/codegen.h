#pragma once
#include <string>
#include <vector>
#include "block.h"

// Gera string de código C equivalente ao script montado.
// Retorna string com quebras de linha \n entre os comandos.
// Chamada a cada frame (eficiente: só regenera se necessário via hash).
std::string codegenRun(const std::vector<Block>& script);
