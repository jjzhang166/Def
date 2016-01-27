#pragma once


#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/raw_ostream.h"

#include "../global.h"
#include "../core/type.h"



namespace def {
namespace compile {

using namespace std;
using namespace llvm;


/**
 * Ŀ�����������
 */
class Target {

public:
    
    LLVMContext & context;
    Module& module;

    // ����
    Target(LLVMContext &, Module &);
    
    // ��� s ��� �� obj �ļ�
    void output(const string &, TargetMachine::CodeGenFileType);

private:
    bool isBinaryOutput(TargetMachine::CodeGenFileType);


};

}
}