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

#ifndef DAWN_SIR_ASTASTOPS_H
#define DAWN_SIR_ASTASTOPS_H

#include "ASTNodeIterator.h"

namespace dawn {
class ASTOps {
public:
  /// @brief Inserts stmt after node pointed by iterator 'node'. Keeps the inout 'node' iterator to
  /// the same position (whether it points to a node or to end()), fixing it to be valid after the
  /// operation. Returns a valid iterator to the inserted node.
  template <ASTNodeIteratorVisitKind onlyFirstLevel>
  static ASTNodeIterator<onlyFirstLevel> insertAfter(std::shared_ptr<Stmt> stmt,
                                                     ASTNodeIterator<onlyFirstLevel>& node);
};
} // namespace dawn

#endif
