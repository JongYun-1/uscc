/**
 * USCC Compiler
 * Jianping Zeng (zeng207@purdue.edu)
 * An iterative backward liveness analysis.
 * This pass intends to compute a set of live-out/in variables for each LLVM basic block
 * and maintain a set of LLVM instructions that are dead---not used by any following others.
*/

#include "Liveness.h"

using namespace std;
using namespace llvm;

bool enableLiveness;

char Liveness::ID = 0;
INITIALIZE_PASS(Liveness, "liveness", "Liveness Analysis", true, true)

FunctionPass *llvm::createLivenessPass() 
{
    return new Liveness();
}

namespace llvm
{
std::set<StringRef> operator+(const std::set<StringRef> & lhs, const std::set<StringRef> & rhs)
{
    std::set<StringRef> ret = lhs;
    for (auto & i : rhs)
        ret.insert(i);
    return ret;
}

void operator+=(std::set<StringRef> & lhs, const std::set<StringRef> & rhs)
{
    for (auto & i : rhs)
        lhs.insert(i);
}

std::set<StringRef> operator-(const std::set<StringRef> & lhs, const std::set<StringRef> & rhs)
{
    std::set<StringRef> ret = lhs;
    for (auto & i : rhs)
        ret.erase(i);
    return ret;
}

void operator-=(std::set<StringRef> & lhs, const std::set<StringRef> & rhs)
{
    for (auto & i : rhs)
        lhs.erase(i);
}
}

void computePostOrder(BasicBlock *entry, set<BasicBlock *> &visited, deque<BasicBlock *> &order) 
{
    visited.insert(entry);
    auto succItr = llvm::succ_begin(entry), end = llvm::succ_end(entry);
    for (; succItr != end; ++succItr) 
        if (!visited.count(*succItr))
        computePostOrder(*succItr, visited, order);
    order.push_back(entry);
}

bool Liveness::runOnFunction(Function &F) 
{
    if (F.empty())
        return false;
    BasicBlock &frontBB = F.front();
    BasicBlock &endBB = F.back();
    assert(!frontBB.empty() && !endBB.empty() && "the front/end basic block must not be empty!");
    // The OUT set of the last block is empty.
    bb2Out[&endBB] = std::set<StringRef>();

    // PA4
    // Step #1: identify program variables.
    for (auto & BB : F)
    {
        for (auto & ins : BB)
        {
            if (ins.getOpcode() == Instruction::Alloca)
                named.insert(ins.getName());
        }
    }

    // Step #2: calculate DEF/USE set for each basic block
    std::map<BasicBlock *, std::set<StringRef>> bb2Use, bb2Def;
    for (auto & BB : F)
    {
        std::set<StringRef> use;
        std::set<StringRef> def;
        for (auto iter = BB.rbegin(); iter != BB.rend(); iter++)
        {
            StoreInst * store = dyn_cast_or_null<StoreInst>(&*iter);
            LoadInst * load = dyn_cast_or_null<LoadInst>(&*iter);
            if (store && named.find(store->getPointerOperand()->getName()) != named.end())
            {
                use.erase(iter->getOperand(1)->getName());
                def.insert(iter->getOperand(1)->getName());
            }
            else if (load && named.find(load->getPointerOperand()->getName()) != named.end())
            {
                use.insert(iter->getOperand(0)->getName());
                def.erase(iter->getOperand(0)->getName());
            }
        }
        bb2Use[&BB] = use;
        bb2Def[&BB] = def;
    }

    // Step #3: compute post order traversal.
    set<BasicBlock *> visited;
    std::deque<BasicBlock *> worklist;
#if 0
    for (auto &bb : F)
        worklist.push_back(&bb);
#else
    computePostOrder(&F.front(), visited, worklist);
#endif

    // Step #4: iterate over control flow graph of the input function until the fixed point.
    unsigned cnt = 0;

    for (auto &i : bb2In)
        i.second.clear();
    for (auto &i : bb2Out)
        i.second.clear();

    bool change = true;
    while (change)
    {
        cnt++;
        change = false;
        for (auto bb : worklist)
        {
            auto & in = bb2In[bb];
            auto & out = bb2Out[bb];
            auto & use = bb2Use[bb];
            auto & def = bb2Def[bb];
            std::set<StringRef> oldIn = in;

            for (auto iter = succ_begin(bb); iter != succ_end(bb); iter++)
                out += bb2In[*iter];

            in = use + (out - def);

            if (oldIn != in)
                change = true;
        }
    }

    // Step #5: output IN/OUT set for each basic block.
    if (enableLiveness) 
    {
        llvm::outs() << "********** Live-in/Live-out information **********\n";
        llvm::outs() << "********** Function: " << F.getName().str() << ", analysis iterates " << cnt << " times\n";
        for (auto &bb : F) 
        {
            llvm::outs() << bb.getName() << ":\n";
            llvm::outs() << "  IN:";
            for (auto &var : bb2In[&bb])
                llvm::outs() << " " << var.substr(0, var.size() - 5);
            llvm::outs() << "\n";
            llvm::outs() << "  OUT:";
            for (auto &var : bb2Out[&bb])
                llvm::outs() << " " << var.substr(0, var.size() - 5);
            llvm::outs() << "\n";
        }
    }
    // Liveness does not change the input function at all.
    return false;
}

bool Liveness::isDead(llvm::Instruction &inst) 
{
    BasicBlock *bb = inst.getParent();
    if (!bb)
        return true;
    if (!bb2Out.count(bb))
        return true;

    // PA4
    StoreInst * st = dyn_cast_or_null<StoreInst>(&inst);
    if (st && named.find(st->getPointerOperand()->getName()) != named.end())
    {
        auto store = dyn_cast_or_null<StoreInst>(&inst);
        auto name = store->getPointerOperand()->getName();
        bool use = false;
        for (auto iter = std::next(BasicBlock::iterator(inst)); iter != bb->end(); iter++)
        {
            StoreInst * store = dyn_cast_or_null<StoreInst>(&*iter);
            LoadInst * load = dyn_cast_or_null<LoadInst>(&*iter);
            if (load)
            {
                auto useName = load->getPointerOperand()->getName();
                if (useName == name)
                {
                    use = true;
                    break;
                }
            }
            if (store)
            {
                auto useName = store->getPointerOperand()->getName();
                if (useName == name)
                    break;
            }
        }
        return !bb2Out[bb].count(name) && !use;
    }
    return false;
}
