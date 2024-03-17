//
//  DeadBlocks.cpp
//  uscc
//
//  Implements Dead Block Removal optimization pass.
//  This removes blocks from the CFG which are unreachable.
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
#include <llvm/IR/CFG.h>
#include <llvm/ADT/DepthFirstIterator.h>
#pragma clang diagnostic pop
#include <set>

using namespace llvm;

namespace uscc
{
namespace opt
{
	
bool DeadBlocks::runOnFunction(Function& F)
{
	bool changed = false;
	
	// PA6: Implement
    std::set<BasicBlock*> visited, unrechable;
    for (auto iter = df_ext_begin(&F.getEntryBlock(), visited); 
        iter != df_ext_end(&F.getEntryBlock(), visited); iter++);

    for (auto iter = F.begin(); iter != F.end(); iter++)
        if (visited.find(&*iter) == visited.end())
            unrechable.emplace(&*iter);

    for (auto & b : unrechable)
    {
        changed = true;
        for (auto iter = succ_begin(b); iter != succ_end(b); iter++)
            iter->removePredecessor(b);
        b->eraseFromParent();
    }
    
	return changed;
}
	
void DeadBlocks::getAnalysisUsage(AnalysisUsage& Info) const
{
	// PA6: Implement
    Info.addRequired<ConstantBranch>();
}

} // opt
} // uscc

char uscc::opt::DeadBlocks::ID = 0;
