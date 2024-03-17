/**
 * USCC Compiler
 * Jianping Zeng (zeng207@purdue.edu)
 * An iterative backward liveness analysis.
 * This pass intends to compute a set of live-out/in variables for each LLVM basic block.
*/

#ifndef USCC_LIVENESS_H
#define USCC_LIVENESS_H

#include "Passes.h"
#include <llvm/IR/Operator.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <vector>
#include <set>

namespace llvm {
// Liveness analysis
class Liveness : public FunctionPass 
{
private:
    // IN[BB] and OUT[BB]
    std::map<BasicBlock *, std::set<StringRef>> bb2In, bb2Out;
    std::set<StringRef> named;
    public:
    static char ID;
    Liveness() : FunctionPass(ID), bb2In(), bb2Out(), named() 
    {
        initializeLivenessPass(*PassRegistry::getPassRegistry());
    }
    virtual bool runOnFunction(llvm::Function &F) override;

    virtual void releaseMemory() override 
    {
        bb2In.clear();
        bb2Out.clear();
        named.clear();
    }

    /**
     * This function is called by other clients to check that a store is dead if its source value
     * is never used by following loads. In this way, we can also remove other instructions directly/indirectly
     * producing the source value.
     * @param inst
     * @return
     */
    bool isDead(Instruction &inst);
};
}

#endif //USCC_LIVENESS_H
