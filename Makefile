CC = gcc
# Flags de compilação
CFLAGS = -Wall -Wextra -g -I$(INC_DIR) -fPIC
LIBS = -lSDL2 -lm

# Diretórios Principais
INC_DIR = include
SRC_DIR = src
BUILD_DIR = build
EXAMPLES_DIR = examples

# Sub-diretórios de Build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

# Arquivos da Lib
LIB_SRC = $(SRC_DIR)/monocordio.c $(SRC_DIR)/mc_presets.c
LIB_OBJ = $(OBJ_DIR)/monocordio.o $(OBJ_DIR)/mc_presets.o
LIB_SO = $(LIB_DIR)/libmonocordio.so

# Targets Finais
ZUM = $(BIN_DIR)/zum
PLING = $(BIN_DIR)/pling
TSC = $(BIN_DIR)/tsc-tsc
KAPOW = $(BIN_DIR)/kapow
PITIU = $(BIN_DIR)/pitiu
SAMSUNG = $(BIN_DIR)/samsung
ORCHESTRA = $(BIN_DIR)/orchestra
METAL_VIOLIN = $(BIN_DIR)/metal_violin

.PHONY: all clean install uninstall directories run_zum run_pling run_tsc run_kapow run_pitiu run_samsung run_orchestra

all: directories $(LIB_SO) $(ZUM) $(PLING) $(TSC) $(KAPOW) $(PITIU) $(SAMSUNG) $(ORCHESTRA) $(METAL_VIOLIN)

directories:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# Compila o objeto da biblioteca (-fPIC via CFLAGS)
# Regra genérica para .c -> .o em build/obj
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Linka a biblioteca dinâmica (-shared)
$(LIB_SO): $(LIB_OBJ)
	@mkdir -p $(dir $@)
	$(CC) -shared -o $@ $(LIB_OBJ) $(LIBS)
	@echo "Biblioteca dinâmica gerada em: $@"

# Compila os exemplos (agora linkando dinamicamente com -lmonocordio)
$(ZUM): $(EXAMPLES_DIR)/zum.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

$(PLING): $(EXAMPLES_DIR)/pling.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

$(TSC): $(EXAMPLES_DIR)/tsc-tsc.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

$(KAPOW): $(EXAMPLES_DIR)/kapow.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

$(PITIU): $(EXAMPLES_DIR)/pitiu.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

$(SAMSUNG): $(EXAMPLES_DIR)/samsung.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

$(ORCHESTRA): $(EXAMPLES_DIR)/orchestra.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

# Limpeza
clean:
	rm -rf $(BUILD_DIR)

# Execução com LD_LIBRARY_PATH configurado
run_zum: $(ZUM)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(ZUM)

run_pling: $(PLING)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(PLING)

run_tsc: $(TSC)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(TSC)

run_kapow: $(KAPOW)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(KAPOW)

run_pitiu: $(PITIU)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(PITIU)

run_samsung: $(SAMSUNG)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(SAMSUNG)

run_orchestra: $(ORCHESTRA)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(ORCHESTRA)

run_metal_violin: $(METAL_VIOLIN)
	LD_LIBRARY_PATH=$(LIB_DIR) ./$(METAL_VIOLIN)

$(METAL_VIOLIN): $(EXAMPLES_DIR)/metal_violin.c $(LIB_SO)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lmonocordio $(LIBS)

# Instalação no sistema
install: $(LIB_SO)
	@echo "Instalando Monocordio..."
	@sudo cp $(LIB_SO) /usr/local/lib/
	@sudo cp $(INC_DIR)/monocordio.h /usr/local/include/
	@sudo cp $(INC_DIR)/mc_presets.h /usr/local/include/
	@sudo ldconfig
	@echo "Instalação concluída!"

uninstall:
	@sudo rm -f /usr/local/lib/libmonocordio.so
	@sudo rm -f /usr/local/include/monocordio.h
	@sudo rm -f /usr/local/include/mc_presets.h
	@sudo ldconfig
	@echo "Monocordio removido."
