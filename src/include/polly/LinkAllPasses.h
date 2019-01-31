//===- polly/LinkAllPasses.h ----------- Reference All Passes ---*- C++ -*-===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header file pulls in all transformation and analysis passes for tools
// like opt and bugpoint that need this functionality.
//
//===----------------------------------------------------------------------===//

#ifndef POLLY_LINKALLPASSES_H
#define POLLY_LINKALLPASSES_H

#include "polly/Config/config.h"
#include <cstdlib>

namespace llvm {
class Pass;
class PassInfo;
class PassRegistry;
class RegionPass;
}

namespace polly {
llvm::Pass *createCodePreparationPass();
llvm::Pass *createDeadCodeElimPass();
llvm::Pass *createDependenceInfoPass();
llvm::Pass *createDOTOnlyPrinterPass();
llvm::Pass *createDOTOnlyViewerPass();
llvm::Pass *createDOTPrinterPass();
llvm::Pass *createDOTViewerPass();
llvm::Pass *createIndependentBlocksPass();
llvm::Pass *createJSONExporterPass();
llvm::Pass *createJSONImporterPass();
llvm::Pass *createPollyCanonicalizePass();
llvm::Pass *createScopDetectionPass();
llvm::Pass *createScopInfoPass();
llvm::Pass *createIslAstInfoPass();
llvm::Pass *createCodeGenerationPass();
llvm::Pass *createIslScheduleOptimizerPass();
llvm::Pass *createTempScopInfoPass();
/* llvm::Pass *createAffineTransformPass(); */
llvm::Pass *createPiecewiseAffineTransformPass();

extern char &IndependentBlocksID;
extern char &CodePreparationID;
}

namespace {
struct PollyForcePassLinking {
  PollyForcePassLinking() {
    // We must reference the passes in such a way that compilers will not
    // delete it all as dead code, even with whole program optimization,
    // yet is effectively a NO-OP. As the compiler isn't smart enough
    // to know that getenv() never returns -1, this will do the job.
    if (std::getenv("bar") != (char *)-1)
      return;

    polly::createCodePreparationPass();
    polly::createDeadCodeElimPass();
    polly::createDependenceInfoPass();
    polly::createDOTOnlyPrinterPass();
    polly::createDOTOnlyViewerPass();
    polly::createDOTPrinterPass();
    polly::createDOTViewerPass();
    polly::createIndependentBlocksPass();
    polly::createJSONExporterPass();
    polly::createJSONImporterPass();
    polly::createScopDetectionPass();
    polly::createScopInfoPass();
    polly::createPollyCanonicalizePass();
    polly::createIslAstInfoPass();
    polly::createCodeGenerationPass();
    polly::createIslScheduleOptimizerPass();
    polly::createTempScopInfoPass();
    /* polly::createAffineTransformPass(); */
    polly::createPiecewiseAffineTransformPass();
  }
} PollyForcePassLinking; // Force link by creating a global definition.
}

namespace llvm {
class PassRegistry;
void initializeCodePreparationPass(llvm::PassRegistry &);
void initializeDeadCodeElimPass(llvm::PassRegistry &);
void initializeIndependentBlocksPass(llvm::PassRegistry &);
void initializeJSONExporterPass(llvm::PassRegistry &);
void initializeJSONImporterPass(llvm::PassRegistry &);
void initializeIslAstInfoPass(llvm::PassRegistry &);
void initializeCodeGenerationPass(llvm::PassRegistry &);
void initializeIslScheduleOptimizerPass(llvm::PassRegistry &);
void initializePollyCanonicalizePass(llvm::PassRegistry &);
void initializeAffineTransformPass(llvm::PassRegistry &);
void initializePiecewiseAffineTransformPass(llvm::PassRegistry &);
}

#endif
