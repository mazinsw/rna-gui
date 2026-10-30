# Redes Neurais - build para macOS, Linux e Windows (MSYS2), GUI com Clay + raylib
#
#   make           GUI (build/NeuralNetwork) e console (build/RedesNeurais)
#   make raylib    baixa e compila o raylib em third_party/, usado no lugar do
#                  raylib do sistema (para quando não há pacote >= 5.5)
#   make app       pacote build/NeuralNetwork.app (somente macOS)
#   make run       executa a GUI
#   make test      executa o console com testes_treinamento.txt
#   make digitos   refaz exemplos/digitos.bin e digitos28.bin e as amostras a
#                  partir das imagens de exemplos/digitos e digitos28
#   make clean     remove build/
#   make distclean remove build/ e o raylib baixado
#
# raylib do sistema: brew install raylib (macOS),
# pacman -S mingw-w64-ucrt-x86_64-raylib (MSYS2) ou o pacote da distribuição.

BUILD   := build
OBJDIR  := $(BUILD)/obj
GEN     := $(BUILD)/gen
UNAME_S := $(shell uname -s)

ifeq ($(OS),Windows_NT)
TARGET_OS := windows
EXE       := .exe
else ifeq ($(UNAME_S),Darwin)
TARGET_OS := macos
else
TARGET_OS := linux
endif

# executáveis que rodam durante o build (o host pode ser outro ao cruzar compilação)
ifneq ($(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)),)
HOST_EXE := .exe
endif
HOSTCXX ?= $(CXX)
WINDRES ?= windres

CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=gnu++20 -Wall
CPPFLAGS += -MMD -MP -isystem third_party/clay -I$(GEN)

ifeq ($(TARGET_OS),macos)
PLATFORM_LIBS := -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo \
                 -framework CoreAudio -framework QuartzCore
SHA256SUM     := shasum -a 256
else ifeq ($(TARGET_OS),windows)
PLATFORM_LIBS := -lopengl32 -lgdi32 -lwinmm
# estático para o .exe não depender das DLLs do MSYS2
GUI_LDFLAGS   := -mwindows -static
CLI_LDFLAGS   := -static
SHA256SUM     := sha256sum
else
PLATFORM_LIBS := -lGL -lm -lpthread -ldl -lrt -lX11
SHA256SUM     := sha256sum
endif

# raylib baixado por "make raylib" tem prioridade sobre o do sistema
RAYLIB_VERSION := 6.2
RAYLIB_SHA256  := df7a13129b1bec9e73a6962efff170beba310ee6eab8f94cd000b7316b4db5ee
RAYLIB_DIR     := third_party/raylib-$(RAYLIB_VERSION)
RAYLIB_LOCAL   := $(RAYLIB_DIR)/src/libraylib.a

ifneq ($(wildcard $(RAYLIB_LOCAL)),)
RAYLIB_CFLAGS := -isystem $(RAYLIB_DIR)/src
RAYLIB_LIBS   := $(RAYLIB_LOCAL) $(PLATFORM_LIBS)
else ifeq ($(TARGET_OS),macos)
# link estático para o .app não depender do Homebrew
RAYLIB_PREFIX := $(shell brew --prefix raylib 2>/dev/null)
RAYLIB_CFLAGS := -isystem $(RAYLIB_PREFIX)/include
RAYLIB_LIBS   := $(RAYLIB_PREFIX)/lib/libraylib.a $(PLATFORM_LIBS)
RAYLIB_CHECK  := test -f "$(RAYLIB_PREFIX)/lib/libraylib.a"
else ifeq ($(TARGET_OS),windows)
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS   := $(shell pkg-config --static --libs raylib 2>/dev/null)
RAYLIB_CHECK  := pkg-config --atleast-version=5.5 raylib
else
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS   := $(shell pkg-config --libs raylib 2>/dev/null)
RAYLIB_CHECK  := pkg-config --atleast-version=5.5 raylib
endif

CORE_SRCS := core/Signal.cpp core/Neuron.cpp training/Training.cpp
CLI_SRCS  := $(CORE_SRCS) test/test_all.cpp test/test_input.cpp test/test_training.cpp
GUI_SRCS  := gui/main.cpp gui/ClayRaylib.cpp gui/Widgets.cpp gui/Input.cpp \
             gui/Platform.cpp gui/Resources.cpp gui/Node.cpp gui/Edge.cpp \
             gui/InputNode.cpp gui/NeuronNode.cpp gui/OutputNode.cpp gui/NeuralNetwork.cpp \
             gui/Execution.cpp gui/Shapes.cpp gui/Icons.cpp gui/Settings.cpp gui/Pixels.cpp

RES_HDRS  := $(GEN)/Roboto-Regular.ttf.h

CLI_OBJS  := $(CLI_SRCS:%.cpp=$(OBJDIR)/%.o)
GUI_OBJS  := $(GUI_SRCS:%.cpp=$(OBJDIR)/%.o)
ifeq ($(TARGET_OS),windows)
GUI_RES   := $(OBJDIR)/res/windows/app.o
endif

DIGITOS_OBJS := $(OBJDIR)/tools/digitos.o $(OBJDIR)/gui/Pixels.o $(OBJDIR)/gui/Platform.o

BIN2C     := $(BUILD)/bin2c$(HOST_EXE)
GUI_BIN   := $(BUILD)/NeuralNetwork$(EXE)
CLI_BIN   := $(BUILD)/RedesNeurais$(EXE)
DIGITOS   := $(BUILD)/digitos$(EXE)
APP       := $(BUILD)/NeuralNetwork.app

.PHONY: all gui console raylib raylib-check app run test digitos clean distclean

all: gui console

gui: $(GUI_BIN)

console: $(CLI_BIN)

$(GUI_BIN): $(GUI_OBJS) $(GUI_RES)
	$(CXX) $(LDFLAGS) $(GUI_LDFLAGS) -o $@ $^ $(RAYLIB_LIBS)

$(CLI_BIN): $(CLI_OBJS)
	$(CXX) $(LDFLAGS) $(CLI_LDFLAGS) -o $@ $^

# usa a redução de imagens da interface, para as amostras serem iguais às
# imagens arrastadas para a janela
$(DIGITOS): $(DIGITOS_OBJS)
	$(CXX) $(LDFLAGS) $(CLI_LDFLAGS) -o $@ $^ $(RAYLIB_LIBS)

digitos: $(DIGITOS)
	./$(DIGITOS) exemplos/digitos exemplos/digitos.bin
	./$(DIGITOS) exemplos/digitos28 exemplos/digitos28.bin 28 16

$(GUI_OBJS): | raylib-check

raylib-check:
ifneq ($(RAYLIB_CHECK),)
	@$(RAYLIB_CHECK) || { echo "raylib >= 5.5 não encontrado: instale o pacote do sistema ou execute 'make raylib'"; exit 1; }
endif

# a regra da GUI vem antes da genérica para ter prioridade
$(OBJDIR)/gui/%.o: gui/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/tools/digitos.o: tools/digitos.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CXXFLAGS) -c $< -o $@

# ícone e informações de versão do executável
$(OBJDIR)/res/windows/app.o: res/windows/app.rc res/img/icon.ico
	@mkdir -p $(@D)
	$(WINDRES) -c 65001 -i $< -O coff -o $@

# fonte embutida no executável
$(OBJDIR)/gui/Resources.o: $(RES_HDRS)

$(BIN2C): tools/bin2c.cpp
	@mkdir -p $(@D)
	$(HOSTCXX) -O2 -o $@ $<

$(GEN)/%.ttf.h: res/fonts/%.ttf $(BIN2C)
	@mkdir -p $(@D)
	$(BIN2C) $< $(subst -,_,$*)_ttf > $@.tmp && mv $@.tmp $@

raylib: $(RAYLIB_LOCAL)

$(RAYLIB_LOCAL):
	@mkdir -p third_party
	curl -fL -o $(RAYLIB_DIR).tar.gz \
		https://github.com/raysan5/raylib/archive/refs/tags/$(RAYLIB_VERSION).tar.gz
	echo "$(RAYLIB_SHA256)  $(RAYLIB_DIR).tar.gz" | $(SHA256SUM) -c -
	tar -xzf $(RAYLIB_DIR).tar.gz -C third_party
	rm -f $(RAYLIB_DIR).tar.gz
	$(MAKE) -C $(RAYLIB_DIR)/src PLATFORM=PLATFORM_DESKTOP

ifeq ($(TARGET_OS),macos)
app: $(APP)
else
app:
	@echo "make app está disponível somente no macOS"; exit 1
endif

$(APP): $(GUI_BIN) res/macos/Info.plist $(BUILD)/AppIcon.icns
	rm -rf $@
	mkdir -p $@/Contents/MacOS $@/Contents/Resources
	cp $(GUI_BIN) $@/Contents/MacOS/NeuralNetwork
	cp res/macos/Info.plist $@/Contents/Info.plist
	cp $(BUILD)/AppIcon.icns $@/Contents/Resources/AppIcon.icns
	codesign --force --sign - $@

$(BUILD)/AppIcon.icns: res/img/icon.ico
	rm -rf $(BUILD)/AppIcon.iconset
	mkdir -p $(BUILD)/AppIcon.iconset
	for size in 16 32 128 256; do \
		sips -s format png -z $$size $$size $< \
			--out $(BUILD)/AppIcon.iconset/icon_$${size}x$${size}.png > /dev/null; \
		sips -s format png -z $$((size * 2)) $$((size * 2)) $< \
			--out $(BUILD)/AppIcon.iconset/icon_$${size}x$${size}@2x.png > /dev/null; \
	done
	iconutil -c icns -o $@ $(BUILD)/AppIcon.iconset

run: $(GUI_BIN)
	./$(GUI_BIN)

test: $(CLI_BIN)
	./$(CLI_BIN) < testes_treinamento.txt

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -rf $(RAYLIB_DIR)

-include $(CLI_OBJS:.o=.d) $(GUI_OBJS:.o=.d) $(OBJDIR)/tools/digitos.d
