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

#ifndef DAWN_IIR_SAPITERATORDECL_H
#define DAWN_IIR_SAPITERATORDECL_H

#include "dawn/SIR/ASTNodeIterator.h"

namespace dawn {

template <ASTNodeIteratorVisitKind onlyFirstLevel,
          ASTNodeIteratorConstQualification isConstIterator =
              ASTNodeIteratorConstQualification::NON_CONST_ITERATOR>
class StatementAccessesPairIterator;

template <ASTNodeIteratorVisitKind onlyFirstLevel>
using StatementAccessesPairConstIterator =
    StatementAccessesPairIterator<onlyFirstLevel,
                                  ASTNodeIteratorConstQualification::CONST_ITERATOR>;

template <ASTNodeIteratorVisitKind onlyFirstLevel,
          ASTNodeIteratorConstQualification isConstIterator =
              ASTNodeIteratorConstQualification::NON_CONST_ITERATOR>
class StatementAccessesPairRange;

template <ASTNodeIteratorVisitKind onlyFirstLevel>
using StatementAccessesPairConstRange =
    StatementAccessesPairRange<onlyFirstLevel, ASTNodeIteratorConstQualification::CONST_ITERATOR>;

} // namespace dawn
#endif
