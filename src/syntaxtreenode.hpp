/** \file syntaxtreenode.hpp
*/

#pragma once

#include "mempoolfactory.hpp"

namespace statpascal {

class TCodeGenerator;

class TSyntaxTreeNode: public TMemoryPoolObject {
public:
    virtual void acceptCodeGenerator (TCodeGenerator &) = 0;
};

}
