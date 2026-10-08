SRC_DIR := src
OBJ_DIR := .obj

CC := gcc
CC_FLAGS := -I$(SRC_DIR)

LD := $(CC)
LD_FLAGS := -lsolv -lsolvext

OBJECTS := \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/argument.o \
	$(OBJ_DIR)/cache.o

NAME_ELF := package_resolver

.PHONY: all clean

all: $(NAME_ELF)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@$(CC) -c $< -o $@ $(CC_FLAGS)
	@echo "CC	$@"


$(NAME_ELF): $(OBJECTS)
	@$(LD) $^ -o $@ $(LD_FLAGS)
	@echo "LD	$(NAME_ELF)"


clean:
	@rm -rf $(OBJ_DIR)
	@echo "Cleaned!"
