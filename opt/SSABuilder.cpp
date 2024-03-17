//
//  SSABuilder.cpp
//  uscc
//
//  Implements SSABuilder class
//  
//---------------------------------------------------------
//  Copyright (c) 2014, Sanjay Madhav
//  All rights reserved.
//
//  This file is distributed under the BSD license.
//  See LICENSE.TXT for details.
//---------------------------------------------------------

#include "SSABuilder.h"
#include "../parse/Symbols.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#include <llvm/IR/Value.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#pragma clang diagnostic pop

#include <list>

using namespace uscc::opt;
using namespace uscc::parse;
using namespace llvm;

// Called when a new function is started to clear out all the data
void SSABuilder::reset()
{
	// PA5: Implement
    for (auto & i : mVarDefs)
        delete i.second;
    mVarDefs.clear();
    for (auto & i : mIncompletePhis)
        delete i.second;
    mIncompletePhis.clear();
    mSealedBlocks.clear();
}

// For a specific variable in a specific basic block, write its value
void SSABuilder::writeVariable(Identifier* var, BasicBlock* block, Value* value)
{
	// PA5: Implement
    (*mVarDefs[block])[var] = value;
}

// Read the value assigned to the variable in the requested basic block
// Will recursively search predecessor blocks if it was not written in this block
Value* SSABuilder::readVariable(Identifier* var, BasicBlock* block)
{
	// PA5: Implement
    auto & subMap = mVarDefs[block];
    if (subMap->find(var) != subMap->end())
        return (*subMap)[var];
    return readVariableRecursive(var, block);
}

// This is called to add a new block to the maps
void SSABuilder::addBlock(BasicBlock* block, bool isSealed /* = false */)
{
	// PA5: Implement
    mVarDefs.emplace(block, new SubMap());
    mIncompletePhis.emplace(block, new SubPHI());
    if (isSealed)
        sealBlock(block);
}

// This is called when a block is "sealed" which means it will not have any
// further predecessors added. It will complete any PHI nodes (if necessary)
void SSABuilder::sealBlock(llvm::BasicBlock* block)
{
	// PA5: Implement
    for (auto & i : *mIncompletePhis[block])
        addPhiOperands(i.first, i.second);
    mSealedBlocks.emplace(block);
}

// Recursively search predecessor blocks for a variable
Value* SSABuilder::readVariableRecursive(Identifier* var, BasicBlock* block)
{
    Value * val;
    if (mSealedBlocks.find(block) == mSealedBlocks.end())
    {
        Instruction * ins;
        if ((ins = block->getFirstNonPHI()) == block->end())
            val = PHINode::Create(var->llvmType(), 0, "Phi", block);
        else
            val = PHINode::Create(var->llvmType(), 0, "Phi", ins);
        (*mIncompletePhis[block])[var] = cast<PHINode>(val);
    }
    else if (block->getSinglePredecessor())
    {
        val = readVariable(var, block->getSinglePredecessor());
    }
    else
    {
        Instruction * ins;
        if ((ins = block->getFirstNonPHI()) == block->end())
            val = PHINode::Create(var->llvmType(), 0, "Phi", block);
        else
            val = PHINode::Create(var->llvmType(), 0, "Phi", ins);
        writeVariable(var, block, val);
        val = addPhiOperands(var, cast<PHINode>(val));
    }
    writeVariable(var, block, val);
	
	return val;
}

// Adds phi operands based on predecessors of the containing block
Value* SSABuilder::addPhiOperands(Identifier* var, PHINode* phi)
{
	// PA5: Implement
    auto block = phi->getParent();
    for (auto it = pred_begin(block); it != pred_end(block); it++)
        phi->addIncoming(readVariable(var, *it), *it);
    return tryRemoveTrivialPhi(phi);
}

// Removes trivial phi nodes
Value* SSABuilder::tryRemoveTrivialPhi(llvm::PHINode* phi)
{
	Value* same = nullptr;
	
	// PA5: Implement
    for (int i = 0; i < phi->getNumIncomingValues(); i++)
    {
        auto op = phi->getIncomingValue(i);
        if (op == same || op == phi)
            continue;
        if (same != nullptr)
            return phi;
        same = op;
    }
    if (same == nullptr)
        same = UndefValue::get(phi->getType());

    std::vector<llvm::User*> users;
    for (auto it = phi->user_begin(); it != phi->user_end(); it++)
    {
        auto use = *it;
        if (use != phi)
            users.emplace_back(use);
    }

    phi->replaceAllUsesWith(same);
    for (auto & b: mVarDefs)
        for (auto & map: *b.second)
            if (map.second == phi) 
                (*mVarDefs[b.first])[map.first] = same;

    phi->eraseFromParent();
    
    for (auto & use : users)
    {
        if (isa<PHINode>(use))
            tryRemoveTrivialPhi(cast<PHINode>(use));
    }

	return same;
}
