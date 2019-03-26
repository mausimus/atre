CC		:= g++-8
#CC		:= clang++
C_FLAGS := -std=c++17 -Wall -Wextra -O3 -g

BIN		:= bin
OBJ		:= obj
SRC		:= src
INCLUDE	:= include
SDL2	:= /usr/include/SDL2
LIB		:= 
PCH		:= src/atre.hpp

LIBRARIES	:= -pthread -latomic -lSDL2 -lstdc++fs

EXECUTABLE	:= atre

src = $(wildcard $(SRC)/*.cpp)
obj = $(addprefix $(OBJ)/, $(notdir $(src:.cpp=.o)))

all: $(BIN)/$(EXECUTABLE)

clean:
	$(RM) $(BIN)/$(EXECUTABLE)
	$(RM) $(OBJ)/*.o

pch:
	$(CC) $(C_FLAGS) -I$(INCLUDE) -w $(PCH)

run: all
	./$(BIN)/$(EXECUTABLE)

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) $(C_FLAGS) -I$(INCLUDE) -I$(SDL2) -o $@ -c $<

$(BIN)/$(EXECUTABLE): $(obj)
	$(CC) $(C_FLAGS) -I$(INCLUDE) -I$(SDL2) $^ -o $@ $(LIBRARIES)
