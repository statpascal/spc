INCDIR = -Isrc
VPATH = src
OBJDIR = obj
TARGET = obj/sp

# CXX = afl-g++
CXX = gcc
# CXX = clang++

SRC = compiler.cpp anymanager.cpp expression.cpp predefined.cpp constant.cpp \
      symboltable.cpp filehandler.cpp codegenerator.cpp datatypes.cpp lexer.cpp statements.cpp config.cpp \
      vectordata.cpp runtime.cpp rng.cpp sp.cpp runtimelib.cpp mempoolfactory.cpp \
      x64generator.cpp x64asm.cpp a64gen.cpp a64asm.cpp tms9900asm.cpp
OBJ = $(patsubst %.cpp,$(OBJDIR)/%.o,$(SRC))

LIBS = -ldl -lffi -lm -lstdc++ -lstdc++fs -pthread
# CPPFLAGS = -g -std=c++20 -pthread -Wall -pedantic 
CPPFLAGS += -g -std=c++20 -O3 -fomit-frame-pointer -falign-functions=16 -march=native -funroll-loops 
ifeq ($(CXX),clang++)
    CPPFLAGS += -Wno-return-type-c-linkage -Wno-logical-op-parentheses
else
    CPPFLAGS += -falign-loops=16
endif


LDFLAGS = -Wl,--export-dynamic


.PHONY: directories tests

all: | directories $(TARGET)

directories: $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)
	
$(TARGET): $(OBJ)
	$(CXX) -o $(TARGET) $(OBJ) $(LIBS) $(LDFLAGS)
	
$(OBJDIR)/%.o: %.cpp
	$(CXX) -c $(INCDIR) $(CPPFLAGS) $< -o $@

tests:
	@printf "Starting regression tests with FPC\n\n"
	@(cd tests/standard; ./runtests.sh ../../$(TARGET)) || (echo "Standard regression tests failed")
	@printf "\nStarting regression tests of SP extensions\n\n"
	@(cd tests/statpascal; ./runtests.sh ../../$(TARGET)) || (echo "StatPascal regression tests failed")
	@if [ ! -f tests/error.tmp ]; then printf "\nAll tests passed\n"; else printf "\nSome tests failed\n"; fi

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(OBJDIR)/sp
	