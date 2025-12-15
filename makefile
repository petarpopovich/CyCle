CXX := g++

MISC := misc
SRC  := src

BIN_ASM := asembler
BIN_LNK := linker
BIN_EMU := emulator

.PHONY: all parser lexer asm link emu clean

all: emu

emu: link
	$(CXX) $(SRC)/emulator.cpp -o $(BIN_EMU)

link: asm
	$(CXX) $(SRC)/linker.cpp -o $(BIN_LNK)

asm: lexer
	$(CXX) $(MISC)/lexer.cpp $(MISC)/parser.cpp $(SRC)/assembler.cpp -o $(BIN_ASM)

lexer: parser
	cd $(MISC) && flex lexer.l

parser:
	cd $(MISC) && bison parser.y

clean:
	rm -f $(BIN_ASM) $(BIN_LNK) $(BIN_EMU) \
	      $(MISC)/parser.cpp $(MISC)/parser.hpp \
	      $(MISC)/lexer.cpp  $(MISC)/lexer.hpp
