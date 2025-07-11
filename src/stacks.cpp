#include "stacks.hpp"

namespace statpascal {

TStack::TStack (const std::size_t stacksize):
  stacksize (stacksize) {
    sp = arena = reinterpret_cast<TStackMemory *> (operator new (stacksize));
}

TStack::~TStack () {
    operator delete (arena);
}

// ---

TCalculatorStack::TCalculatorStack ():
  sp (0), stacksize (1024) {
    stack.resize (stacksize);
    sp = &stack [0];
    sptop = sp + stacksize - 1;
}

void TCalculatorStack::resize () {
    stacksize *= 2;
    stack.resize (stacksize);
    sp = &stack [stacksize / 2 - 1];
    sptop = &stack [0] + stacksize - 1;
}

}
