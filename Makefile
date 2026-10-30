# Redes Neurais - build para macOS, Linux e Windows (MSYS2), GUI com Clay + raylib
#
#   make           GUI (build/NeuralNetwork), linha de comando das redes da GUI
#                  (build/rna) e console do formato texto (build/RedesNeurais)
#   make raylib    baixa e compila o raylib em third_party/, usado no lugar do
#                  raylib do sistema (para quando não há pacote >= 5.5)
#   make app       pacote build/NeuralNetwork.app (somente macOS)
#   make run       executa a GUI
#   make test      executa o console com testes_treinamento.txt
#   make check     testes do treino, sem janela; com dados/mnist também o MNIST
#   make digitos   refaz exemplos/digitos.bin e digitos28.bin e as amostras a
#                  partir das imagens de exemplos/digitos e digitos28
#   make mnist     baixa o MNIST para dados/mnist e refaz exemplos/mnist.bin,
#                  treinado com ele
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
# CommandLineToArgvW, para os argumentos em UTF-8
CONSOLE_LIBS  := -lshell32
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

# rede, execução e treino, sem o raylib
TRAINING_SRCS := training/Network.cpp training/Dataset.cpp training/Trainer.cpp
CLI_SRCS  := $(TRAINING_SRCS) console/main.cpp cli/Console.cpp
GUI_SRCS  := $(TRAINING_SRCS) gui/main.cpp gui/ClayRaylib.cpp gui/Widgets.cpp gui/Input.cpp \
             gui/Platform.cpp gui/Resources.cpp gui/Draw.cpp gui/Number.cpp gui/NeuralNetwork.cpp \
             gui/Shapes.cpp gui/Icons.cpp gui/Settings.cpp gui/Pixels.cpp
# a linha de comando lê as imagens com o raylib, sem a janela
RNA_SRCS  := $(TRAINING_SRCS) cli/main.cpp cli/Console.cpp gui/Platform.cpp gui/Number.cpp \
             gui/Pixels.cpp
DIGITOS_SRCS := $(TRAINING_SRCS) tools/digitos.cpp gui/Platform.cpp gui/Pixels.cpp
CHECK_SRCS := $(TRAINING_SRCS) test/check.cpp

RES_HDRS  := $(GEN)/Roboto-Regular.ttf.h

CLI_OBJS  := $(CLI_SRCS:%.cpp=$(OBJDIR)/%.o)
GUI_OBJS  := $(GUI_SRCS:%.cpp=$(OBJDIR)/%.o)
RNA_OBJS  := $(RNA_SRCS:%.cpp=$(OBJDIR)/%.o)
DIGITOS_OBJS := $(DIGITOS_SRCS:%.cpp=$(OBJDIR)/%.o)
CHECK_OBJS := $(CHECK_SRCS:%.cpp=$(OBJDIR)/%.o)
ifeq ($(TARGET_OS),windows)
GUI_RES   := $(OBJDIR)/res/windows/app.o
endif

BIN2C     := $(BUILD)/bin2c$(HOST_EXE)
GUI_BIN   := $(BUILD)/NeuralNetwork$(EXE)
CLI_BIN   := $(BUILD)/RedesNeurais$(EXE)
DIGITOS   := $(BUILD)/digitos$(EXE)
RNA_BIN   := $(BUILD)/rna$(EXE)
CHECK_BIN := $(BUILD)/check$(EXE)

# MNIST, fora do git: as 60 mil imagens de treino e as 10 mil de teste
MNIST_DIR   := dados/mnist
MNIST_URL   := https://ossci-datasets.s3.amazonaws.com/mnist
MNIST_FILES := $(addprefix $(MNIST_DIR)/,train-images-idx3-ubyte train-labels-idx1-ubyte \
               t10k-images-idx3-ubyte t10k-labels-idx1-ubyte)
SHA256_train-images-idx3-ubyte := 440fcabf73cc546fa21475e81ea370265605f56be210a4024d2ca8f203523609
SHA256_train-labels-idx1-ubyte := 3552534a0a558bbed6aed32b30c495cca23d567ec52cac8be1a0730e8010255c
SHA256_t10k-images-idx3-ubyte  := 8d422c7b0a1c1c79245a5bcf07fe86e33eeafee792b84584aec276f5a2dbc4e6
SHA256_t10k-labels-idx1-ubyte  := f7ae60f92e00ec6debd23a6088c31dbd2371eca3ffa0defaefb259924204aec6
APP       := $(BUILD)/NeuralNetwork.app

.PHONY: all gui console rna raylib raylib-check app run test check digitos mnist clean distclean

all: gui console rna

gui: $(GUI_BIN)

console: $(CLI_BIN)

rna: $(RNA_BIN)

$(GUI_BIN): $(GUI_OBJS) $(GUI_RES)
	$(CXX) $(LDFLAGS) $(GUI_LDFLAGS) -o $@ $^ $(RAYLIB_LIBS)

$(CLI_BIN): $(CLI_OBJS)
	$(CXX) $(LDFLAGS) $(CLI_LDFLAGS) -o $@ $^ $(CONSOLE_LIBS)

$(CHECK_BIN): $(CHECK_OBJS)
	$(CXX) $(LDFLAGS) $(CLI_LDFLAGS) -o $@ $^

$(RNA_BIN): $(RNA_OBJS)
	$(CXX) $(LDFLAGS) $(CLI_LDFLAGS) -o $@ $^ $(RAYLIB_LIBS) $(CONSOLE_LIBS)

# usa a redução de imagens da interface, para as amostras serem iguais às
# imagens arrastadas para a janela
$(DIGITOS): $(DIGITOS_OBJS)
	$(CXX) $(LDFLAGS) $(CLI_LDFLAGS) -o $@ $^ $(RAYLIB_LIBS)

digitos: $(DIGITOS)
	./$(DIGITOS) exemplos/digitos exemplos/digitos.bin
	./$(DIGITOS) exemplos/digitos28 exemplos/digitos28.bin 28 16

# a rede do notebook, 784 -> 256 ReLU -> 10 softmax, treinada por 10 épocas
mnist: $(MNIST_FILES) $(DIGITOS) $(RNA_BIN)
	./$(DIGITOS) $(MNIST_DIR) exemplos/mnist.bin 28 256
	./$(RNA_BIN) treinar exemplos/mnist.bin $(MNIST_DIR)

$(MNIST_DIR)/%:
	@mkdir -p $(@D)
	curl -fL -o $@.gz $(MNIST_URL)/$*.gz
	echo "$(SHA256_$*)  $@.gz" | $(SHA256SUM) -c -
	gunzip -f $@.gz

# o console não usa o raylib
$(filter $(OBJDIR)/gui/%,$(GUI_OBJS)) $(OBJDIR)/cli/main.o $(OBJDIR)/tools/digitos.o: | raylib-check

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

$(OBJDIR)/cli/%.o: cli/%.cpp
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

check: $(CHECK_BIN)
	./$(CHECK_BIN) $(wildcard $(MNIST_DIR))

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -rf $(RAYLIB_DIR)

-include $(sort $(CLI_OBJS:.o=.d) $(GUI_OBJS:.o=.d) $(RNA_OBJS:.o=.d) $(DIGITOS_OBJS:.o=.d) \
	$(CHECK_OBJS:.o=.d))
