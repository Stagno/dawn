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

#ifndef DAWN_SIR_ASTASTNODEITERATOR_H
#define DAWN_SIR_ASTASTNODEITERATOR_H

#include "ASTStmt.h"
#include "dawn/Support/IteratorRange.h"
#include <list>
#include <type_traits>

namespace dawn {

/// @brief (Abstract) Pre-order visiting iterator through the Stmts of one or more ASTs.
/// Specializations should specify on which ASTs to iterate and in which order.
/// @ingroup sir
template <typename GeneratorType, bool onlyFirstLevel, typename Enable = void>
class ASTNodeIterator {
public:
  virtual ~ASTNodeIterator() = 0;
};

#define ASTNodeIterator_Stmt                                                                       \
  ASTNodeIterator<ASTNode, onlyFirstLevel,                                                         \
                  typename std::enable_if<std::is_base_of<Stmt, ASTNode>::value>::type>
#define ASTVoidIterator_Stmt ASTVoidIterator<ASTNode, onlyFirstLevel>

/// @brief Pre-order visiting iterator through the Stmts of an AST. Generated from a root Stmt.
/// Depending on onlyFirstLevel parameter the visit is/isn't limited to the first level (also
/// excluding root) of the AST.
/// @ingroup sir
template <typename ASTNode, bool onlyFirstLevel>
class ASTNodeIterator_Stmt {
  friend class ASTOps;
  template <typename T, bool B, typename E>
  friend class ASTNodeIterator;

protected:
  Stmt& rootStmt_;
  std::unique_ptr<ASTNodeIterator> restIterator_ = nullptr;
  Stmt::StmtRangeType::iterator childrenIterator_ = nullptr;
  const bool isTop_;

public:
  static ASTNodeIterator CreateInstance(Stmt& root) {
    return std::move(*(CreateInstance<true>(root).release()));
  }

  ASTNodeIterator(ASTNodeIterator&&) = default;
  ASTNodeIterator(const ASTNodeIterator& o) = delete;
  ASTNodeIterator& operator=(ASTNodeIterator&& o) { return *this = std::move(o); }
  ASTNodeIterator& operator=(const ASTNodeIterator& o) = delete;

  ASTNodeIterator clone() const;

  ASTNodeIterator& operator++();
  bool operator==(const ASTNodeIterator& other) const;
  inline bool operator!=(const ASTNodeIterator& other) const { return !(*this == other); }

  ASTNodeIterator& setToEnd();

  Stmt& operator*() const;

  inline bool isVisitingRoot() const { return childrenIterator_ == nullptr; }

  inline bool isEnd() const { return isChildrenIteratorAtEnd() && restIterator_->isEnd(); }

  inline bool isVoidIter() const { return false; }

  inline bool isTop() const { return isTop_; }
  /// @brief returns the root of the AST covered by this iterator
  Stmt& getASTRoot() const { return rootStmt_; }

  /// @brief returns a new ASTNodeIterator pointing to the same node of the first
  /// level of the AST but of switched visit type (only first-level or full).
  ASTNodeIterator<ASTNode, !onlyFirstLevel,
                  typename std::enable_if<std::is_base_of<Stmt, ASTNode>::value>::type>
  toggleOnlyFirstLevelVisiting() const;

  // List of ancestors from tree root to parent of rootStmt_ (empty if this is the tree root)
  inline const std::list<Stmt*> getAncestors() const {
    return isVisitingRoot() ? std::list<Stmt*>()
                            : std::list<Stmt*>(&rootStmt_) + restIterator_->getAncestors();
  }

protected:
  ASTNodeIterator(Stmt& root, bool isTop) : rootStmt_(root), isTop_(isTop) {
    // If iterating only through the first level, need to skip root.
    if(isTop && onlyFirstLevel)
      operator++();
  }

  template <bool isTopLevel>
  static std::unique_ptr<ASTNodeIterator> CreateInstance(Stmt& root);

private:
  inline bool isChildrenIteratorAtEnd() const {
    return childrenIterator_ == rootStmt_.getChildren().end();
  }
  void setupRestIterator();
};

/// @brief Iterator that only vists the root node and not its children.
/// @ingroup sir
template <typename ASTNode, bool onlyFirstLevel>
class ASTVoidIterator : public ASTNodeIterator_Stmt {
  bool isEnd_ = false;
  friend class ASTNodeIterator_Stmt;

public:
  ASTVoidIterator(ASTVoidIterator&&) = default;
  ASTVoidIterator(const ASTVoidIterator& o) = delete;
  ASTVoidIterator& operator=(ASTVoidIterator&& o) { *this = std::move(o); }
  ASTVoidIterator& operator=(const ASTVoidIterator& o) = delete;

  ASTVoidIterator clone() const;

  ASTVoidIterator& operator++();
  bool operator==(const ASTVoidIterator& other) const;
  inline ASTVoidIterator& setToEnd() {
    isEnd_ = true;
    return *this;
  }
  inline Stmt& operator*() { return this->rootStmt_; }
  inline bool isEnd() const { return isEnd_; }
  inline bool isVoidIter() const { return true; }

protected:
  ASTVoidIterator(Stmt& root, bool isTop) : ASTNodeIterator_Stmt(root, isTop) {}
};

template <typename RootNode, bool onlyFirstLevel>
class ASTRange : public IteratorRange<ASTNodeIterator<RootNode, onlyFirstLevel>> {
public:
  using Iterator = ASTNodeIterator<RootNode, onlyFirstLevel>;
  ASTRange(RootNode& root)
      : IteratorRange<Iterator>(std::move(Iterator::CreateInstance(root)),
                                std::move(Iterator::CreateInstance(root).setToEnd())) {}
  ASTRange(const Iterator singleton)
      : IteratorRange<Iterator>(std::move(Iterator(singleton.clone())),
                                std::move(++Iterator(singleton.clone()))) {}

  ASTRange(ASTRange&&) = default;
  ASTRange(const ASTRange&) = delete;
  ASTRange& operator=(ASTRange&& o) { *this = std::move(o); }
  ASTRange& operator=(const ASTRange&) = delete;

  inline Stmt& operator[](int index) const { return *std::next(this->firstIt_, index); }
};

template <typename RootNode, bool onlyFirstLevel>
ASTRange<RootNode, onlyFirstLevel> iterateASTOver(RootNode& root) {
  return ASTRange<RootNode, onlyFirstLevel>(root);
}

} // namespace dawn

namespace std {
template <typename GeneratorType, bool onlyFirstLevel, typename Enable>
struct iterator_traits<dawn::ASTNodeIterator<GeneratorType, onlyFirstLevel, Enable>> {
  using difference_type =
      typename iterator_traits<typename dawn::BlockStmt::StatementList::iterator>::difference_type;
  using value_type = dawn::Stmt;
  using pointer = dawn::Stmt*;
  using reference = dawn::Stmt&;
  using iterator_category = std::input_iterator_tag;
};

} // namespace std

//===------------------------------------------------------------------------------------------===//
//
//
//     Definitions of templated classes declared before in this file
//
//
//===------------------------------------------------------------------------------------------===//

namespace dawn {

//
//  Definition of ASTNodeIterator_Stmt
//

template <typename ASTNode, bool onlyFirstLevel>
ASTNodeIterator_Stmt ASTNodeIterator_Stmt::clone() const {
  ASTNodeIterator o(rootStmt_, isTop_);
  o.childrenIterator_ = childrenIterator_;
  o.restIterator_ = restIterator_ == nullptr ? nullptr
                                             : std::unique_ptr<ASTNodeIterator>(new ASTNodeIterator(
                                                   std::move(restIterator_->clone())));
  return o;
}

template <typename ASTNode, bool onlyFirstLevel>
ASTNodeIterator_Stmt& ASTNodeIterator_Stmt::operator++() {
  if(isVisitingRoot()) {
    childrenIterator_ = rootStmt_.getChildren().begin();
    setupRestIterator();
    return *this;
  }
  if(!isEnd()) {
    if(restIterator_->isEnd()) {
      childrenIterator_++;
      setupRestIterator();
    } else {
      (*restIterator_).operator++();
      if(restIterator_->isEnd())
        this->operator++();
    }
  }

  return *this;
}

template <typename ASTNode, bool onlyFirstLevel>
bool ASTNodeIterator_Stmt::operator==(const ASTNodeIterator_Stmt& other) const {
  return (isVoidIter() == other.isVoidIter()) && isTop_ == other.isTop_ &&
         (&getASTRoot() == &other.getASTRoot()) && (childrenIterator_ == other.childrenIterator_) &&
         (&*restIterator_ == &*(other.restIterator_));
}

template <typename ASTNode, bool onlyFirstLevel>
ASTNodeIterator_Stmt& ASTNodeIterator_Stmt::setToEnd() {
  childrenIterator_ = rootStmt_.getChildren().end() - 1;
  setupRestIterator();
  restIterator_->setToEnd();
  childrenIterator_ = rootStmt_.getChildren().end();
  return *this;
}

template <typename ASTNode, bool onlyFirstLevel>
Stmt& ASTNodeIterator_Stmt::operator*() const {
  if(isVisitingRoot())
    return rootStmt_;
  return **restIterator_;
}

template <typename ASTNode, bool onlyFirstLevel>
ASTNodeIterator<ASTNode, !onlyFirstLevel,
                typename std::enable_if<std::is_base_of<Stmt, ASTNode>::value>::type>
ASTNodeIterator<ASTNode, onlyFirstLevel,
                typename std::enable_if<std::is_base_of<Stmt, ASTNode>::value>::type>::
    toggleOnlyFirstLevelVisiting() const {
  ASTNodeIterator<ASTNode, !onlyFirstLevel,
                  typename std::enable_if<std::is_base_of<Stmt, ASTNode>::value>::type>
      output(rootStmt_, true);
  output.childrenIterator_ = childrenIterator_;
  if(output.childrenIterator_)
    output.setupRestIterator();
  return output;
}

template <typename ASTNode, bool onlyFirstLevel>
template <bool isTopLevel>
std::unique_ptr<ASTNodeIterator_Stmt> ASTNodeIterator_Stmt::CreateInstance(Stmt& root) {
  if(root.getChildren().empty())
    return std::unique_ptr<ASTVoidIterator_Stmt>(new ASTVoidIterator_Stmt(root, isTopLevel));
  using ConditionalType =
      typename std::conditional<onlyFirstLevel && (!isTopLevel), ASTVoidIterator<ASTNode, true>,
                                ASTNodeIterator>::type;
  return std::unique_ptr<ConditionalType>(new ConditionalType(root, isTopLevel));
}

template <typename ASTNode, bool onlyFirstLevel>
void ASTNodeIterator_Stmt::setupRestIterator() {
  if(isChildrenIteratorAtEnd())
    return;

  restIterator_ = CreateInstance<false>(**childrenIterator_);
}

//
//  Definition of ASTVoidIterator_Stmt
//

template <typename ASTNode, bool onlyFirstLevel>
ASTVoidIterator_Stmt ASTVoidIterator_Stmt::clone() const {
  ASTVoidIterator o(this->rootStmt_, this->isTop_);
  o.isEnd_ = this->isEnd_;
  return o;
}

template <typename ASTNode, bool onlyFirstLevel>
ASTVoidIterator_Stmt& ASTVoidIterator_Stmt::operator++() {
  isEnd_ = true;
  return *this;
}

template <typename ASTNode, bool onlyFirstLevel>
bool ASTVoidIterator_Stmt::operator==(const ASTVoidIterator& other) const {
  return (&this->getASTRoot() == &other.getASTRoot()) && this->isEnd_ == other.isEnd_;
}

} // namespace dawn

#endif
