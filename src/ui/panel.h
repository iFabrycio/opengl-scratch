#pragma once

#include <string>

// Retângulo sólido
void uiRect(float x, float y, float w, float h,
            float r, float g, float b, float a = 1.0f);

// Retângulo com borda arredondada (aproximação com triângulos, raio pequeno ~4px)
void uiRoundRect(float x, float y, float w, float h,
                 float r, float g, float b, float cornerR = 6.0f);

// Texto com fonte bitmap GLUT
// font: 0=HELVETICA_12, 1=HELVETICA_18, 2=TIMES_24
void uiText(float x, float y, const std::string& s, int font = 1,
            float r = 0.92f, float g = 0.92f, float b = 0.95f);

// Largura aproximada de um texto (para centralizar)
float uiTextWidth(const std::string& s, int font = 1);

// Linha horizontal separadora
void uiHLine(float x, float y, float w,
             float r = 0.25f, float g = 0.27f, float b = 0.35f);

// Botão clicável — retorna true se foi clicado (mouseX/mouseY em coord do painel)
// pressed: true = desenha pressionado (mais escuro)
bool uiButton(float x, float y, float w, float h,
              const std::string& label,
              float br, float bg, float bb,
              bool pressed = false);

// Slider horizontal — value é o valor atual; min/max são os limites
// Retorna o novo valor baseado em mouseX se dragging=true
void uiSlider(float x, float y, float w,
              float value, float minV, float maxV,
              const std::string& label);

// Ícone de categoria (círculo colorido com letra)
void uiCategoryIcon(float cx, float cy, float radius,
                    float r, float g, float b, char letter);

// Botão com ícone geométrico — icon: 'U'=seta cima, 'D'=seta baixo, 'X'=fechar
void uiIconButton(float x, float y, float w, float h, char icon,
                  float br, float bg, float bb);

// Linha tracejada horizontal (para guias visuais)
void uiDashedHLine(float x, float y, float w,
                   float r, float g, float b,
                   float dashLen = 6.f, float gapLen = 4.f);

// Linha tracejada vertical
void uiDashedVLine(float x, float y, float h,
                   float r, float g, float b,
                   float dashLen = 6.f, float gapLen = 4.f);
