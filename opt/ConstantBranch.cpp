//
//  ContantBranch.cpp
//  uscc
//
//  Implements Constant Branch Folding opt pass.
//  This converts conditional branches on constants to
//  unconditional branches.
//
//---------------------------------------------------------
//  Copyright (c) 2014, Sanjay Madhav
//  All rights reserved.
//
//  This file is distributed under the BSD license.
//  See LICENSE.TXT for details.
//---------------------------------------------------------
#include "Passes.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#pragma clang diagnostic pop
#include <set>

using namespace llvm;

namespace uscc
{
namespace opt
{
	
bool ConstantBranch::runOnFunction(Function& F)
{
	bool changed = false;
	
	// PA6: Implement
    std::set<BranchInst*> removeSet;
	
    // Loop through each block
    for (Function::iterator blockIter = F.begin(); blockIter != F.end(); blockIter++)
    {
        // Loop through instructions in block
        for (BasicBlock::iterator instrIter = blockIter->begin(); 
            instrIter != blockIter->end(); instrIter++)
        {
            if (BranchInst* branch = dyn_cast<BranchInst>(instrIter))
            {
                if (branch->isConditional())
                {
                    if (isa<ConstantInt>(branch->getCondition()))
                        removeSet.emplace(branch);
                }
            }
        }
    }

    for (auto & i : removeSet)
    {
        changed = true;
        ConstantInt * value = cast<ConstantInt>(i->getCondition());
        if (value->getValue().getBoolValue())
        {
            llvm::IRBuilder<> builder(i->getParent());
            builder.CreateBr(i->getSuccessor(0));
            i->getSuccessor(1)->removePredecessor(i->getParent());
        }
        else
        {
            llvm::IRBuilder<> builder(i->getParent());
            builder.CreateBr(i->getSuccessor(1));
            i->getSuccessor(0)->removePredecessor(i->getParent());
        }
        i->eraseFromParent();
    }
	return changed;
}

void ConstantBranch::getAnalysisUsage(AnalysisUsage& Info) const
{
	// PA6: Implement
    Info.addRequired<ConstantOps>();
}
	
} // opt
} // uscc

char uscc::opt::ConstantBranch::ID = 0;
