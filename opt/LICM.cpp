//
//  LICM.cpp
//  uscc
//
//  Implements basic loop invariant code motion
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
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/ValueTracking.h>
#pragma clang diagnostic pop

using namespace llvm;

namespace uscc
{
namespace opt
{
	

bool LICM::isSafeToHoistInstr(llvm::Instruction * ins) const
{
    return (mCurrLoop->hasLoopInvariantOperands(ins) &&
        isSafeToSpeculativelyExecute(ins) &&
     (isa<BinaryOperator>(ins) || isa<CastInst>(ins) || isa<SelectInst>(ins) 
        || isa<GetElementPtrInst>(ins) || isa<CmpInst>(ins)));
}

void LICM::hoistInstr(llvm::Instruction * ins)
{
    auto preheader = mCurrLoop->getLoopPreheader();
    ins->moveBefore(preheader->getTerminator());
    mChanged = true;
}

void LICM::hoistPreOrder(llvm::DomTreeNode * node)
{
    auto b = node->getBlock();

    if (mLoopInfo->getLoopFor(b) == mCurrLoop)
    {
        for (auto iter = b->begin(); iter != b->end();)
        {
            auto ins = &*iter;
            iter++;
            if (isSafeToHoistInstr(ins))
                hoistInstr(ins);
        }
    }

    for (auto & c : node->getChildren())
        hoistPreOrder(c);
}

bool LICM::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM)
{
	mChanged = false;
	
	// PA6: Implement
    // Save the current loop 
    mCurrLoop = L; 
    // Grab the loop info
    mLoopInfo = &getAnalysis<LoopInfo>();
    // Grab the dominator tree
    mDomTree = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    hoistPreOrder(mDomTree->getNode(L->getHeader()));

	return mChanged;
}

void LICM::getAnalysisUsage(AnalysisUsage &Info) const
{
	// PA6: Implement
    // LICM does not modify the CFG 
    Info.setPreservesCFG();
    // Execute after dead blocks have been removed 
    Info.addRequired<DeadBlocks>();
    // Use the built-in Dominator tree and loop info passes 
    Info.addRequired<DominatorTreeWrapperPass>(); 
    Info.addRequired<LoopInfo>();
}
	
} // opt
} // uscc

char uscc::opt::LICM::ID = 0;
