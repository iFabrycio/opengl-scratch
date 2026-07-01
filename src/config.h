#pragma once

// Dimensões da janela
const int WIN_W = 1200;
const int WIN_H = 700;

// Larguras das colunas
const int PALETTE_W = 200;   // paleta de blocos
const int SCRIPT_W  = 280;   // script montado
// VIEWPORT_W = WIN_W - PALETTE_W - SCRIPT_W = 720

// Altura do rodapé de código
const int FOOTER_H  = 140;

// Cores (floats 0-1) — tema claro amigável para crianças
// Background dos painéis UI
const float UI_BG_R = 0.94f, UI_BG_G = 0.95f, UI_BG_B = 0.97f;
// Background do viewport 3D (claro, harmoniza com o tema)
const float VP_BG_R = 0.90f, VP_BG_G = 0.91f, VP_BG_B = 0.94f;
// Separadores suaves
const float SEP_R   = 0.74f, SEP_G   = 0.76f, SEP_B   = 0.82f;
// Texto primário (quase preto)
const float TXT_R   = 0.14f, TXT_G   = 0.15f, TXT_B   = 0.20f;
// Texto secundário (cinza médio)
const float TXT2_R  = 0.42f, TXT2_G  = 0.44f, TXT2_B  = 0.52f;
