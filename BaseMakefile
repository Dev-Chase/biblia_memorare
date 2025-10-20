# Folders
INCLUDE :=  include
OBJ :=  obj
SRC :=  src
BIN :=  bin
LIB :=  lib


# Sub-Folders
ifeq ($(OS),Windows_NT)
MAIN	:= main.exe
SRCDIRS	:= $(SRC)
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
FIXPATH = $(subst /,\,$1)
RM			:= del /q /f
RMRF 		:= rmdir /s /q
MD	:= mkdir
else
MAIN	:= main
SRCDIRS	:= $(shell find $(SRC) -type d)
OBJDIRS		:= $(patsubst $(SRC)%, $(OBJ)%, $(SRCDIRS))
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
RMRF := rm -rf
MD	:= mkdir -p
endif

# Names
MAIN := main
MAIN_SRC := $(SRC)/$(MAIN).c
MAIN_BIN := $(BIN)/$(MAIN)

# Flags
INCLUDES_FLAG := $(patsubst %,-I%,$(INCLUDEDIRS:%/=%))
STD := c11
CC := clang
CFLAGS := $(INCLUDES_FLAG) -std=$(STD) -g -Wall -Wextra

# Files
LIBS := $(wildcard $(patsubst %,%/*, $(LIBDIRS)))
INCLUDES := $(wildcard $(patsubst %,%/*, $(INCLUDEDIRS)))
SRCS := $(wildcard $(patsubst %,%/*.c,$(SRCDIRS)))
OBJS := $(patsubst $(SRC)%,$(OBJ)%,$(SRCS:.c=.o))

all: $(BIN) $(MAIN_BIN)
	@echo Executing 'all' complete

$(BIN):
	$(MD) $(BIN)

run: all
	$(MAIN_BIN)

exec:
	$(MAIN_BIN)

bear-make: clean
	@bear -- make

$(OBJ)/%.o: $(SRC)/%.c $(INCLUDES) | $(OBJDIRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/$(MAIN).o: $(MAIN_SRC) $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

$(MAIN_BIN): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $^ -o $@


clean:
	rm -rf *.dSYM $(OBJ)/* $(BIN)/*
