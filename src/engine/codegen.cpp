#include "codegen.h"
#include <sstream>
#include <cstdio>

// Formata float com 2 casas e sufixo f
static std::string ff(float v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2ff", v);
    return buf;
}

// Formata int (para fatias/pilhas)
static std::string fi(float v) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", (int)v);
    return buf;
}

std::string codegenRun(const std::vector<Block>& script) {
    if (script.empty())
        return "// Adicione blocos ao script para ver o codigo aqui";

    std::ostringstream out;

    // Grupos para comentários de seção:
    // 0=camera, 1=iluminacao, 2=cena(objetos+transforms)
    // Imprime comentário de seção só na primeira vez que aparece cada grupo
    bool sectionPrinted[3] = { false, false, false };

    auto printSection = [&](int grp, const char* title) {
        if (!sectionPrinted[grp]) {
            if (out.tellp() > 0) out << "\n";
            out << "// === " << title << " ===\n";
            sectionPrinted[grp] = true;
        }
    };

    for (const Block& b : script) {
        std::string line;

        switch (b.type) {
        // ---- Camera ----
        case BLK_CAM_PERSPECTIVE:
            printSection(0, "Configuracao da Camera");
            line = "gluPerspective(" + ff(b.params[0]) + ", aspect, "
                 + ff(b.params[1]) + ", " + ff(b.params[2]) + ");";
            break;

        case BLK_CAM_ORTHO: {
            printSection(0, "Configuracao da Camera");
            float z = b.params[0];
            line = "glOrtho(" + ff(-z) + "*aspect, " + ff(z) + "*aspect, "
                 + ff(-z) + ", " + ff(z) + ", 0.10f, 100.00f);";
            break;
        }

        case BLK_CAM_LOOKAT:
            printSection(0, "Configuracao da Camera");
            line = "gluLookAt("
                 + ff(b.params[0]) + ", " + ff(b.params[1]) + ", " + ff(b.params[2]) + ",  "
                 + ff(b.params[3]) + ", " + ff(b.params[4]) + ", " + ff(b.params[5]) + ",  "
                 + "0.00f, 1.00f, 0.00f);";
            break;

        // ---- Iluminacao ----
        case BLK_LIGHT_ENABLE:
            printSection(1, "Iluminacao");
            line = "glEnable(GL_LIGHTING);\nglEnable(GL_LIGHT0);\nglEnable(GL_NORMALIZE);";
            break;

        case BLK_LIGHT_DIR: {
            printSection(1, "Iluminacao");
            float i = b.params[3];
            line = "{ GLfloat pos[] = {" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", "
                 + ff(b.params[2]) + ", 0.00f};\n"
                 + "  GLfloat dif[] = {" + ff(i) + ", " + ff(i) + ", " + ff(i) + ", 1.00f};\n"
                 + "  glLightfv(GL_LIGHT0, GL_POSITION, pos);\n"
                 + "  glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif); }";
            break;
        }

        case BLK_LIGHT_POINT: {
            printSection(1, "Iluminacao");
            float i = b.params[3];
            line = "{ GLfloat pos[] = {" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", "
                 + ff(b.params[2]) + ", 1.00f};\n"
                 + "  GLfloat dif[] = {" + ff(i) + ", " + ff(i) + ", " + ff(i) + ", 1.00f};\n"
                 + "  glLightfv(GL_LIGHT0, GL_POSITION, pos);\n"
                 + "  glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif); }";
            break;
        }

        case BLK_MATERIAL: {
            printSection(1, "Iluminacao");
            line = "{ GLfloat mat[] = {" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", "
                 + ff(b.params[2]) + ", 1.00f};\n"
                 + "  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mat);\n"
                 + "  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, " + ff(b.params[3]) + "); }";
            break;
        }

        case BLK_AMBIENT_COLOR: {
            printSection(1, "Iluminacao");
            line = "{ GLfloat amb[] = {" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", "
                 + ff(b.params[2]) + ", 1.00f};\n"
                 + "  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb); }";
            break;
        }

        // ---- Transformacoes ----
        case BLK_TRANSLATE:
            printSection(2, "Cena");
            line = "glTranslatef(" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", " + ff(b.params[2]) + ");";
            break;

        case BLK_ROTATE:
            printSection(2, "Cena");
            line = "glRotatef(" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", "
                 + ff(b.params[2]) + ", " + ff(b.params[3]) + ");";
            break;

        case BLK_SCALE:
            printSection(2, "Cena");
            line = "glScalef(" + ff(b.params[0]) + ", " + ff(b.params[1]) + ", " + ff(b.params[2]) + ");";
            break;

        case BLK_PUSH_POP:
            printSection(2, "Cena");
            line = "glPushMatrix();  // isolando transformacoes\n// ... proximos blocos ...\nglPopMatrix();";
            break;

        // ---- Objetos ----
        case BLK_CUBE:
            printSection(2, "Cena");
            line = "glutSolidCube(" + ff(b.params[0]) + ");";
            break;

        case BLK_SPHERE:
            printSection(2, "Cena");
            line = "gluSphere(quadric, " + ff(b.params[0]) + ", "
                 + fi(b.params[1]) + ", " + fi(b.params[2]) + ");";
            break;

        case BLK_TEAPOT:
            printSection(2, "Cena");
            line = "glutSolidTeapot(" + ff(b.params[0]) + ");";
            break;

        case BLK_PLANE:
            printSection(2, "Cena");
            line = "// Plano " + ff(b.params[0]) + " x " + ff(b.params[1]) + "\n"
                 + "glBegin(GL_QUADS);\n"
                 + "  glNormal3f(0.00f, 1.00f, 0.00f);\n"
                 + "  glVertex3f(" + ff(-b.params[0]*0.5f) + ", 0.00f, " + ff(-b.params[1]*0.5f) + ");\n"
                 + "  glVertex3f(" + ff( b.params[0]*0.5f) + ", 0.00f, " + ff(-b.params[1]*0.5f) + ");\n"
                 + "  glVertex3f(" + ff( b.params[0]*0.5f) + ", 0.00f, " + ff( b.params[1]*0.5f) + ");\n"
                 + "  glVertex3f(" + ff(-b.params[0]*0.5f) + ", 0.00f, " + ff( b.params[1]*0.5f) + ");\n"
                 + "glEnd();";
            break;

        case BLK_CONE:
            printSection(2, "Cena");
            line = "gluCylinder(quadric, " + ff(b.params[0]) + ", 0.00f, "
                 + ff(b.params[1]) + ", " + fi(b.params[2]) + ", 4);";
            break;

        case BLK_CYLINDER:
            printSection(2, "Cena");
            line = "gluCylinder(quadric, " + ff(b.params[0]) + ", " + ff(b.params[0]) + ", "
                 + ff(b.params[1]) + ", " + fi(b.params[2]) + ", 4);";
            break;

        case BLK_TEXTURE:
            printSection(2, "Cena");
            line = std::string("glBindTexture(GL_TEXTURE_2D, tex_") + b.strParam + ");\n"
                 + "glEnable(GL_TEXTURE_2D);";
            break;

        case BLK_BEZIER:
            printSection(2, "Cena");
            line = "// Curva de Bezier cubica\n"
                   "// P0=(" + ff(b.params[0]) + "," + ff(b.params[1]) + ") "
                   "P1=(" + ff(b.params[2]) + "," + ff(b.params[3]) + ") "
                   "P2=(" + ff(b.params[4]) + "," + ff(b.params[5]) + ") "
                   "P3=(1.50f,0.00f)\n"
                   "GLfloat ctrlPts[4][3] = { {" + ff(b.params[0]) + "," + ff(b.params[1]) + ",0.00f},\n"
                   "  {" + ff(b.params[2]) + "," + ff(b.params[3]) + ",0.00f},\n"
                   "  {" + ff(b.params[4]) + "," + ff(b.params[5]) + ",0.00f},\n"
                   "  {1.50f,0.00f,0.00f} };\n"
                   "glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 4, &ctrlPts[0][0]);\n"
                   "glEnable(GL_MAP1_VERTEX_3);\n"
                   "glMapGrid1f(50, 0.0f, 1.0f);\n"
                   "glEvalMesh1(GL_LINE, 0, 50);";
            break;

        default:
            continue;
        }

        if (!b.enabled) {
            // Comenta cada linha do bloco desabilitado
            out << "// [desabilitado]\n";
            std::istringstream iss(line);
            std::string l;
            while (std::getline(iss, l))
                out << "// " << l << "\n";
        } else {
            out << line << "\n";
        }
    }

    return out.str();
}
