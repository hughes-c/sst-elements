// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _PARSER_H
#define _PARSER_H

#define DEBUG

// TODO Temporary -- need to fix
# ifdef PIC
# undef PIC
# endif

#include <sst/core/sst_config.h>

#include <vector>
#include <string>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

# include "llvm/IR/Function.h"
# include "llvm/IR/PassManager.h"
# include "llvm/IR/Module.h"
# include "llvm/Passes/PassBuilder.h"
# include "llvm/Passes/PassPlugin.h"
# include "llvm/Analysis/LoopAnalysisManager.h"

#include "graph/graph.h"
#include "pes/peList.h"
#include "llyrTypes.h"

namespace SST {
namespace Llyr {

struct alignas(int64_t) ParserEdgeProperties : EdgeProperties
{
   llvm::Value*  value_;
   int64_t       const_;
};

struct alignas(int64_t) CDFGVertex
{
   llvm::Instruction*  instruction_;
   std::string         valueName_;

   Addr                memAddr_;
   bool                haveConst_;
   int64_t             intConst_;
   float               floatConst_;
   double              doubleConst_;

   std::string         instructionName_;
   std::string         sizeType_;
};

typedef LlyrGraph< CDFGVertex* > CDFG;
typedef LlyrGraph< llvm::BasicBlock* > BBGraph;

class Parser
{
public:
    Parser(const std::string& offloadString, SST::Output* output, Addr startingAddr) :
    output_(output), offloadString_(offloadString), currentAddr_ (startingAddr)
    {
        vertexList_ = new std::map< llvm::BasicBlock*, std::vector< CDFGVertex* > >;

        defNode_ = new std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >;
        useNode_ = new std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >;

        flowGraph_ = new std::map< llvm::BasicBlock*, CDFG* >;
        bbGraph_   = new BBGraph;
    }

    ~Parser() {};

    void generateAppGraph( std::string functionName );

protected:
    llvm::ModuleAnalysisManager MAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::LoopAnalysisManager LAM;

private:
    SST::Output* output_;
    std::string  offloadString_;
    std::string  offloadTarget_;

    Addr         currentAddr_;

    BBGraph* bbGraph_;
    CDFG*    functionGraph_;

    llvm::Module* mod_;

    std::map< llvm::BasicBlock*, CDFG* >* flowGraph_;
    std::map< llvm::BasicBlock*, std::vector< CDFGVertex* > >* vertexList_;

    std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >* defNode_;
    std::map< llvm::BasicBlock*, std::map< CDFGVertex*, std::vector< llvm::Instruction* >* >* >* useNode_;

    void runAnalysisOnFunction(llvm::Function* func);

    void generatebBasicBlockGraph(llvm::Function* func);
    void expandBBGraph(llvm::Function* func);
    void assembleGraph();
    void mergeGraphs();

    void fixOperations();

    void printVertex ( const CDFGVertex* ) const;
    void printCDFG( const std::string fileName ) const;
    void printPyMapper( const std::string fileName ) const;

};

} // namespace LLyr
} // namespace SST

#endif /* _PARSER_H */
