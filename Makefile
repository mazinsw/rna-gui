# Redes Neurais - build para macOS e Linux (GUI com Clay + raylib)
#
#   make          GUI (build/NeuralNetwork) e console (build/RedesNeurais)
#   make app      pacote build/NeuralNetwork.app (somente macOS)
#   make run      executa a GUI
#   make test     executa o console com testes_treinamento.txt
#   make clean    remove build/
#
# Requer raylib: brew install raylib (macOS) ou o pacote da distribuição.

BUILD   := build
OBJDIR  := $(BUILD)/obj
GEN     := $(BUILD)/gen
UNAME_S := $(shell uname -s)

CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++20 -Wall
CPPFLAGS += -MMD -MP -isystem third_party/clay -I$(GEN)

ifeq ($(UNAME_S),Darwin)
# link estático para o .app não depender do Homebrew
RAYLIB_PREFIX ?= $(shell brew --prefix raylib 2>/dev/null)
RAYLIB_CFLAGS ?= -isystem $(RAYLIB_PREFIX)/include
RAYLIB_LIBS   ?= $(RAYLIB_PREFIX)/lib/libraylib.a -framework Cocoa -framework IOKit \
                 -framework OpenGL -framework CoreVideo -framework CoreAudio -framework QuartzCore
else
RAYLIB_CFLAGS ?= $(shell pkg-config --cflags raylib)
RAYLIB_LIBS   ?= $(shell pkg-config --libs raylib) -lm -lpthread -ldl
endif

CORE_SRCS := core/Signal.cpp core/Neuron.cpp training/Training.cpp
CLI_SRCS  := $(CORE_SRCS) test/test_all.cpp test/test_input.cpp test/test_training.cpp
GUI_SRCS  := gui/main.cpp gui/ClayRaylib.cpp gui/Widgets.cpp gui/Input.cpp \
             gui/Platform.cpp gui/Resources.cpp gui/Node.cpp gui/Edge.cpp \
             gui/InputNode.cpp gui/NeuronNode.cpp gui/OutputNode.cpp gui/NeuralNetwork.cpp

IMAGES    := neuron input output funcao_linear funcao_degree funcao_bipolar \
             funcao_logistic funcao_hiperbolic
RES_HDRS  := $(IMAGES:%=$(GEN)/%.png.h) $(GEN)/Roboto-Regular.ttf.h

CLI_OBJS  := $(CLI_SRCS:%.cpp=$(OBJDIR)/%.o)
GUI_OBJS  := $(GUI_SRCS:%.cpp=$(OBJDIR)/%.o)

GUI_BIN   := $(BUILD)/NeuralNetwork
CLI_BIN   := $(BUILD)/RedesNeurais
APP       := $(BUILD)/NeuralNetwork.app

.PHONY: all gui console app run test clean

all: gui console

gui: $(GUI_BIN)

console: $(CLI_BIN)

$(GUI_BIN): $(GUI_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(RAYLIB_LIBS)

$(CLI_BIN): $(CLI_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# a regra da GUI vem antes da genérica para ter prioridade
$(OBJDIR)/gui/%.o: gui/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# imagens e fonte embutidas no executável
$(OBJDIR)/gui/Resources.o: $(RES_HDRS)

$(GEN)/%.png.h: res/img/%.png
	@mkdir -p $(@D)
	cd $(<D) && xxd -i $(<F) > $(CURDIR)/$@

$(GEN)/%.ttf.h: res/fonts/%.ttf
	@mkdir -p $(@D)
	cd $(<D) && xxd -i $(<F) > $(CURDIR)/$@

app: $(APP)

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

-include $(CLI_OBJS:.o=.d) $(GUI_OBJS:.o=.d)
