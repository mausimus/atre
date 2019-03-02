CC		:= g++
C_FLAGS := -std=c++17 -Wall -Wextra -g

BIN		:= bin
OBJ		:= obj
SRC		:= src
INCLUDE	:= include
LIB		:= lib
PCH		:= src/atre.hpp

LIBRARIES	:=

ifeq ($(OS),Windows_NT)
EXECUTABLE	:= atre.exe
else
EXECUTABLE	:= atre
endif

src = $(wildcard $(SRC)/*.cpp)
obj = $(addprefix $(OBJ)/, $(notdir $(src:.cpp=.o)))

all: $(BIN)/$(EXECUTABLE)

clean:
	$(RM) $(BIN)/$(EXECUTABLE)
	$(RM) $(OBJ)/*.o

pch:
	$(CC) $(C_FLAGS) -I$(INCLUDE) -L$(LIB) -w $(PCH)

run: all
	./$(BIN)/$(EXECUTABLE)

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) $(C_FLAGS) -I$(INCLUDE) -L$(LIB) -o $@ -c $<

$(BIN)/$(EXECUTABLE): $(obj)
	$(CC) $(C_FLAGS) -I$(INCLUDE) -L$(LIB) $^ -o $@ $(LIBRARIES)
