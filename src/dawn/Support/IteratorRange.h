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
#ifndef DAWN_SUPPORT_ITERATORRANGE_H
#define DAWN_SUPPORT_ITERATORRANGE_H
#include <utility>

namespace dawn {

template <typename IteratorType>
class IteratorRange {
protected:
  IteratorType firstIt_, secondIt_;

public:
  IteratorRange(IteratorType firstIt, IteratorType secondIt)
      : firstIt_(std::move(firstIt)), secondIt_(std::move(secondIt)) {}
  IteratorRange clone() const { return IteratorRange(firstIt_.clone(), secondIt_.clone()); }
  IteratorType begin() const { return std::move(firstIt_.clone()); }
  IteratorType end() const { return std::move(secondIt_.clone()); }
};

} // namespace dawn

#endif
