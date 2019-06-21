//===--------------------------------------------------------------------------------*- C++ -*-===//
//                          _
//                         | |
//                       __| | __ ___      ___ ___
//                      / _` |/ _` \ \ /\ / / '_  |
//                     | (_| | (_| |\ V  V /| | | |
//                      \__,_|\__,_| \_/\_/ |_| |_| - Compiler Toolchain
//
//
//  This file is distributed under the MIT License (MIT).
//  See LICENSE.txt for details.
//
//===------------------------------------------------------------------------------------------===//
// TODO iir_restructuring RECOVER
// TODO REMOVE
#include "dawn/Compiler/DawnCompiler.h"

namespace dawn {

DawnCompiler::DawnCompiler(Options* options) : diagnostics_(make_unique<DiagnosticsEngine>()) {
  options_ = options ? make_unique<Options>(*options) : make_unique<Options>();
}

std::unique_ptr<OptimizerContext> DawnCompiler::runOptimizer(std::shared_ptr<SIR> const& SIR) {

  // Initialize optimizer
  std::unique_ptr<OptimizerContext> optimizer =
      make_unique<OptimizerContext>(getDiagnostics(), getOptions(), SIR);

  return optimizer;
}

std::unique_ptr<codegen::TranslationUnit> DawnCompiler::compile(const std::shared_ptr<SIR>& SIR) {

  return nullptr;
}

const DiagnosticsEngine& DawnCompiler::getDiagnostics() const { return *diagnostics_.get(); }
DiagnosticsEngine& DawnCompiler::getDiagnostics() { return *diagnostics_.get(); }

const Options& DawnCompiler::getOptions() const { return *options_.get(); }
Options& DawnCompiler::getOptions() { return *options_.get(); }

} // namespace dawn
