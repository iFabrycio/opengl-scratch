CXX     = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Isrc -D_USE_MATH_DEFINES

# Detecta sistema operacional
OS := $(shell uname 2>/dev/null || echo Windows)

ifeq ($(OS), Darwin)
    TARGET   = opengl-scratch
    LDFLAGS  = -framework OpenGL -framework GLUT
    CXXFLAGS += -Wno-deprecated-declarations
    EXE_EXT  =
    RM_CMD   = rm -f
else ifeq ($(findstring MINGW,$(OS))$(findstring MSYS,$(OS))$(findstring CYGWIN,$(OS)), )
    # Linux
    TARGET   = opengl-scratch
    LDFLAGS  = -lGL -lGLU -lglut
    EXE_EXT  =
    RM_CMD   = rm -f
else
    # Windows (MinGW/MSYS2) — instalar: pacman -S mingw-w64-x86_64-freeglut
    TARGET   = opengl-scratch.exe
    LDFLAGS  = -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm
    CXXFLAGS += -DFREEGLUT_STATIC
    EXE_EXT  = .exe
    RM_CMD   = rm -f
endif

# Coleta todos os .cpp em src/, src/ui/, src/engine/, src/viewport/
SRCS := $(wildcard src/*.cpp) \
        $(wildcard src/ui/*.cpp) \
        $(wildcard src/engine/*.cpp) \
        $(wildcard src/viewport/*.cpp)

OBJS := $(SRCS:.cpp=.o)

# Regra padrão
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compilação de cada .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Roda o programa
run: all
	./$(TARGET)

# Limpa artefatos
clean:
	$(RM_CMD) $(OBJS) $(TARGET)

.PHONY: all run clean
