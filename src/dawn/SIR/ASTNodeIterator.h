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

enum class ASTNodeIteratorVisitKind : bool {
  FULL_AST_VISIT = false,
  ONLY_FIRST_LEVEL_VISIT = true
};

enum class ASTNodeIteratorConstQualification : bool {
  NON_CONST_ITERATOR = false,
  CONST_ITERATOR = true
};

/// @brief Pre-order visiting iterator through the Stmts of an AST. Generated from a root Stmt.
/// Depending on onlyFirstLevel parameter the visit is/isn't limited to the first level (also
/// excluding root) of the AST.
/// @ingroup sir
template <ASTNodeIteratorVisitKind onlyFirstLevel>
class ASTNodeIteratorImpl {
  friend class ASTOps; // TODO iir_restructuring : remove
  template <ASTNodeIteratorVisitKind K>
  friend class ASTNodeIteratorImpl;
  template <ASTNodeIteratorVisitKind K>
  friend class SubtreeIteratorImpl;
  template <ASTNodeIteratorVisitKind K>
  friend class VoidIteratorImpl;

public:
  static std::unique_ptr<ASTNodeIteratorImpl> CreateInstance(const std::shared_ptr<Stmt>& root);

  ASTNodeIteratorImpl(ASTNodeIteratorImpl&&) = default;
  ASTNodeIteratorImpl(const ASTNodeIteratorImpl& o) = delete;
  virtual ASTNodeIteratorImpl& operator=(ASTNodeIteratorImpl&& o) = delete;
  ASTNodeIteratorImpl& operator=(const ASTNodeIteratorImpl& o) = delete;
  virtual ~ASTNodeIteratorImpl() = default;

  virtual std::unique_ptr<ASTNodeIteratorImpl> clone() const = 0;

  virtual std::unique_ptr<ASTNodeIteratorImpl> operator++() = 0;
  virtual bool operator==(const ASTNodeIteratorImpl& other) const = 0;
  inline bool operator!=(const ASTNodeIteratorImpl& other) const { return !(*this == other); }

  virtual Stmt& operator*() const = 0;
  /// @brief sets the iterator to end and returns it
  virtual ASTNodeIteratorImpl& setToEnd() = 0;
  virtual bool isVisitingRoot() const = 0;
  virtual bool isEnd() const = 0;
  virtual bool isVoidIter() const = 0;

  inline bool isTop() const { return isTop_; }
  /// @brief returns the root of the AST covered by this iterator
  inline const std::shared_ptr<Stmt>& getASTRoot() const { return *rootStmtIt_; }

  /// @brief returns a new ASTNodeIteratorImpl pointing to the same node of the first
  /// level of the AST but of switched visit type (only first-level or full).
  virtual std::unique_ptr<ASTNodeIteratorImpl<
      static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>>
  toggleOnlyFirstLevelVisiting() const = 0;

  /// @brief List of ancestors from tree root to parent of rootStmtIt_ (empty if this is the tree
  /// root)
  virtual std::list<Stmt::StmtRangeType::iterator> getAncestors() const = 0;

protected:
  Stmt::StmtRangeType::iterator rootStmtIt_;
  const bool isTop_;

  static std::unique_ptr<ASTNodeIteratorImpl>
  CreateTopLevelInstance(Stmt::StmtRangeType::iterator rootStmtIt);
  static std::unique_ptr<ASTNodeIteratorImpl>
  CreateRestInstance(Stmt::StmtRangeType::iterator rootStmtIt);

  ASTNodeIteratorImpl(Stmt::StmtRangeType::iterator rootStmtIt, const bool isTop)
      : rootStmtIt_(rootStmtIt), isTop_(isTop) {}
};

template <ASTNodeIteratorVisitKind onlyFirstLevel,
          ASTNodeIteratorConstQualification isConstIterator =
              ASTNodeIteratorConstQualification::NON_CONST_ITERATOR>
class ASTNodeIterator {
  friend class ASTOps; // TODO iir_restructuring : remove
  template <ASTNodeIteratorVisitKind K, ASTNodeIteratorConstQualification I>
  friend class ASTNodeIterator;
  std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> impl_;

public:
  static ASTNodeIterator CreateInstance(const std::shared_ptr<Stmt>& root) {
    return ASTNodeIterator(ASTNodeIteratorImpl<onlyFirstLevel>::CreateInstance(root));
  }

  ASTNodeIterator(ASTNodeIterator&& o) { impl_ = std::move(o.impl_); } // Steal the implementation
  ASTNodeIterator(const ASTNodeIterator& o) = delete;
  ASTNodeIterator& operator=(ASTNodeIterator&& o) {
    return *this = ASTNodeIterator(std::move(o.impl_));
  }
  ASTNodeIterator& operator=(const ASTNodeIterator& o) = delete;

  inline ASTNodeIterator clone() const { return ASTNodeIterator(impl_->clone()); }

  inline ASTNodeIterator operator++() { return ASTNodeIterator(impl_->operator++()); }
  inline bool operator==(const ASTNodeIterator& other) const {
    return impl_->operator==(*other.impl_);
  }
  inline bool operator!=(const ASTNodeIterator& other) const { return !(*this == other); }

  inline typename std::conditional<static_cast<bool>(isConstIterator), const Stmt&, Stmt&>::type
  operator*() const {
    return impl_->operator*();
  }
  /// @brief sets the iterator to end and returns it
  inline ASTNodeIterator& setToEnd() {
    impl_->setToEnd();
    return *this;
  }
  inline bool isVisitingRoot() const { return impl_->isVisitingRoot(); }
  inline bool isEnd() const { return impl_->isEnd(); }
  inline bool isVoidIter() const { return impl_->isVoidIter(); }

  inline bool isTop() const { return impl_->isTop(); }
  /// @brief returns the root of the AST covered by this iterator
  inline const std::shared_ptr<Stmt>& getASTRoot() const { return impl_->getASTRoot(); }

  /// @brief returns a new ASTNodeIterator pointing to the same node of the first
  /// level of the AST but of switched visit type (only first-level or full).
  inline ASTNodeIterator<static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>
  toggleOnlyFirstLevelVisiting() const {
    return ASTNodeIterator<static_cast<ASTNodeIteratorVisitKind>(
        !static_cast<bool>(onlyFirstLevel))>(impl_->toggleOnlyFirstLevelVisiting());
  }

  /// @brief List of ancestors from tree root to parent of rootStmtIt_ (empty if this is the tree
  /// root)
  inline std::list<Stmt::StmtRangeType::iterator> getAncestors() const {
    return impl_->getAncestors();
  }

private:
  ASTNodeIterator(std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>&& impl)
      : impl_(std::move(impl)) {}
};
template <ASTNodeIteratorVisitKind onlyFirstLevel>
ASTNodeIterator<onlyFirstLevel> makeASTNodeIterator(const std::shared_ptr<Stmt>& root) {
  return ASTNodeIterator<onlyFirstLevel>::CreateInstance(root);
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
using ASTNodeConstIterator =
    ASTNodeIterator<onlyFirstLevel, ASTNodeIteratorConstQualification::CONST_ITERATOR>;

/// @brief Iterator that vists the root node's subtree.
/// @ingroup sir
template <ASTNodeIteratorVisitKind onlyFirstLevel>
class SubtreeIteratorImpl : public ASTNodeIteratorImpl<onlyFirstLevel> {
  friend class ASTOps; // TODO iir_restructuring : remove
  template <ASTNodeIteratorVisitKind K>
  friend class ASTNodeIteratorImpl;
  template <ASTNodeIteratorVisitKind K>
  friend class SubtreeIteratorImpl;

public:
  SubtreeIteratorImpl(SubtreeIteratorImpl&&) = default;
  SubtreeIteratorImpl(const SubtreeIteratorImpl& o) = delete;
  SubtreeIteratorImpl& operator=(SubtreeIteratorImpl&& o) { return *this = std::move(o); }
  SubtreeIteratorImpl& operator=(const SubtreeIteratorImpl& o) = delete;
  ~SubtreeIteratorImpl() override = default;

  std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> clone() const;

  std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> operator++();
  bool operator==(const ASTNodeIteratorImpl<onlyFirstLevel>& other) const;
  bool operator==(const SubtreeIteratorImpl& other) const;
  inline bool operator!=(const SubtreeIteratorImpl& other) const { return !(*this == other); }

  SubtreeIteratorImpl& setToEnd();

  Stmt& operator*() const;
  inline bool isVisitingRoot() const { return restIterator_ == nullptr; }
  inline bool isEnd() const {
    return restIterator_->rootStmtIt_ == (*this->rootStmtIt_)->getChildren().end();
  }
  inline bool isVoidIter() const { return false; }

  std::unique_ptr<ASTNodeIteratorImpl<
      static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>>
  toggleOnlyFirstLevelVisiting() const;

  inline std::list<Stmt::StmtRangeType::iterator> getAncestors() const {
    std::list<Stmt::StmtRangeType::iterator> list({this->rootStmtIt_});
    list.splice(list.end(), restIterator_->getAncestors());
    return isVisitingRoot() ? std::list<Stmt::StmtRangeType::iterator>() : list;
  }

protected:
  std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> restIterator_ =
      nullptr; // Lazy allocation. nullptr means visiting root.

  SubtreeIteratorImpl(Stmt::StmtRangeType::iterator rootStmtIt, const bool isTop)
      : ASTNodeIteratorImpl<onlyFirstLevel>(rootStmtIt, isTop) {
    if(isTop && static_cast<bool>(onlyFirstLevel))
      operator++(); // If iterating only through the first level, need to skip root.
  }
};

/// @brief Iterator that only vists the root node and not its children.
/// @ingroup sir
template <ASTNodeIteratorVisitKind onlyFirstLevel>
class VoidIteratorImpl : public ASTNodeIteratorImpl<onlyFirstLevel> {
  bool isEnd_ = false;
  friend class ASTOps; // TODO iir_restructuring : remove
  template <ASTNodeIteratorVisitKind O>
  friend class ASTNodeIteratorImpl;
  template <ASTNodeIteratorVisitKind O>
  friend class VoidIteratorImpl;

public:
  VoidIteratorImpl(VoidIteratorImpl&&) = default;
  VoidIteratorImpl(const VoidIteratorImpl& o) = delete;
  VoidIteratorImpl& operator=(VoidIteratorImpl&& o) = default;
  VoidIteratorImpl& operator=(const VoidIteratorImpl& o) = delete;
  ~VoidIteratorImpl() override = default;

  std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> clone() const;

  std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> operator++();
  bool operator==(const ASTNodeIteratorImpl<onlyFirstLevel>& other) const;
  bool operator==(const VoidIteratorImpl& other) const;
  inline bool operator!=(const VoidIteratorImpl& other) const { return !(*this == other); }

  inline VoidIteratorImpl& setToEnd() {
    isEnd_ = true;
    return *this;
  }
  inline Stmt& operator*() const { return **this->rootStmtIt_; }
  inline bool isVisitingRoot() const { return !isEnd_; }
  inline bool isEnd() const { return isEnd_; }
  inline bool isVoidIter() const { return true; }

  std::unique_ptr<ASTNodeIteratorImpl<
      static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>>
  toggleOnlyFirstLevelVisiting() const;

  inline std::list<Stmt::StmtRangeType::iterator> getAncestors() const {
    return isVisitingRoot() ? std::list<Stmt::StmtRangeType::iterator>()
                            : std::list<Stmt::StmtRangeType::iterator>({this->rootStmtIt_});
  }

protected:
  VoidIteratorImpl(Stmt::StmtRangeType::iterator rootStmtIt, const bool isTop)
      : ASTNodeIteratorImpl<onlyFirstLevel>(rootStmtIt, isTop) {
    if(isTop && static_cast<bool>(onlyFirstLevel))
      setToEnd(); // If iterating only through the first level, need to skip root.
  }
};

template <ASTNodeIteratorVisitKind onlyFirstLevel,
          ASTNodeIteratorConstQualification isConstIterator =
              ASTNodeIteratorConstQualification::NON_CONST_ITERATOR>
class ASTRange : public IteratorRange<ASTNodeIterator<onlyFirstLevel, isConstIterator>> {
public:
  using Iterator = ASTNodeIterator<onlyFirstLevel, isConstIterator>;
  ASTRange(const std::shared_ptr<Stmt>& root)
      : IteratorRange<Iterator>(std::move(Iterator::CreateInstance(root)),
                                std::move(Iterator::CreateInstance(root).setToEnd())) {}
  ASTRange(const Iterator singleton)
      : IteratorRange<Iterator>(std::move(Iterator(singleton.clone())),
                                std::move(++Iterator(singleton.clone()))) {}

  ASTRange(ASTRange&&) = default;
  ASTRange(const ASTRange&) = delete;
  ASTRange& operator=(ASTRange&& o) { *this = std::move(o); }
  ASTRange& operator=(const ASTRange&) = delete;
};

template <ASTNodeIteratorVisitKind onlyFirstLevel>
using ASTConstRange = ASTRange<onlyFirstLevel, ASTNodeIteratorConstQualification::CONST_ITERATOR>;

template <ASTNodeIteratorVisitKind onlyFirstLevel>
ASTRange<onlyFirstLevel> iterateASTOver(const std::shared_ptr<Stmt>& root) {
  return ASTRange<onlyFirstLevel>(root);
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
ASTConstRange<onlyFirstLevel> constIterateASTOver(const std::shared_ptr<Stmt>& root) {
  return ASTConstRange<onlyFirstLevel>(root);
}

} // namespace dawn

namespace std {
template <dawn::ASTNodeIteratorVisitKind onlyFirstLevel,
          dawn::ASTNodeIteratorConstQualification isConstIterator>
struct iterator_traits<dawn::ASTNodeIterator<onlyFirstLevel, isConstIterator>> {
  using difference_type =
      typename iterator_traits<typename dawn::BlockStmt::StatementList::iterator>::difference_type;
  using value_type = typename std::conditional<static_cast<bool>(isConstIterator), const dawn::Stmt,
                                               dawn::Stmt>::type;
  using pointer = typename std::conditional<static_cast<bool>(isConstIterator), const dawn::Stmt*,
                                            dawn::Stmt*>::type;
  using reference = typename std::conditional<static_cast<bool>(isConstIterator), const dawn::Stmt&,
                                              dawn::Stmt&>::type;
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
//  Definition of ASTNodeIteratorImpl
//

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>
ASTNodeIteratorImpl<onlyFirstLevel>::CreateInstance(const std::shared_ptr<Stmt>& root) {

  Stmt::StmtRangeType singletonContainer(const_cast<std::shared_ptr<Stmt>&>(root));
  return CreateTopLevelInstance(singletonContainer.begin());
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>
ASTNodeIteratorImpl<onlyFirstLevel>::CreateTopLevelInstance(
    Stmt::StmtRangeType::iterator rootStmtIt) {
  if((*rootStmtIt)->getChildren().empty())
    return std::unique_ptr<VoidIteratorImpl<onlyFirstLevel>>(
        new VoidIteratorImpl<onlyFirstLevel>(rootStmtIt, true));
  return std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>(
      new SubtreeIteratorImpl<onlyFirstLevel>(rootStmtIt, true));
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>
ASTNodeIteratorImpl<onlyFirstLevel>::CreateRestInstance(Stmt::StmtRangeType::iterator rootStmtIt) {
  if((*rootStmtIt)->getChildren().empty())
    return std::unique_ptr<VoidIteratorImpl<onlyFirstLevel>>(
        new VoidIteratorImpl<onlyFirstLevel>(rootStmtIt, false));
  using ASTNodeIteratorImplType =
      typename std::conditional<static_cast<bool>(onlyFirstLevel),
                                VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>,
                                SubtreeIteratorImpl<onlyFirstLevel>>::type;
  return std::unique_ptr<ASTNodeIteratorImplType>(new ASTNodeIteratorImplType(rootStmtIt, false));
}

//
//  Definition of SubtreeIteratorImpl
//

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>
SubtreeIteratorImpl<onlyFirstLevel>::clone() const {
  SubtreeIteratorImpl* o = new SubtreeIteratorImpl(this->rootStmtIt_, this->isTop_);
  o->restIterator_ = restIterator_ == nullptr ? nullptr : std::move(restIterator_->clone());
  return std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>(o);
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> SubtreeIteratorImpl<onlyFirstLevel>::
operator++() {
  if(isVisitingRoot()) {

    restIterator_ = ASTNodeIteratorImpl<onlyFirstLevel>::CreateRestInstance(
        (*this->rootStmtIt_)->getChildren().begin());
    return this->clone();
  }
  if(!isEnd()) {
    if(restIterator_->isEnd()) {
      restIterator_ = ASTNodeIteratorImpl<onlyFirstLevel>::CreateRestInstance(
          std::next(restIterator_->rootStmtIt_, 1));
    } else {
      restIterator_->operator++();
      if(restIterator_->isEnd())
        this->operator++();
    }
  }

  return this->clone();
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
bool SubtreeIteratorImpl<onlyFirstLevel>::
operator==(const ASTNodeIteratorImpl<onlyFirstLevel>& other) const {
  if(other.isVoidIter())
    return false;
  return (*this == *dynamic_cast<SubtreeIteratorImpl const*>(&other));
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
bool SubtreeIteratorImpl<onlyFirstLevel>::operator==(const SubtreeIteratorImpl& other) const {
  return this->isTop_ == other.isTop_ && this->rootStmtIt_ == other.rootStmtIt_ &&
         *restIterator_ == *(other.restIterator_);
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
SubtreeIteratorImpl<onlyFirstLevel>& SubtreeIteratorImpl<onlyFirstLevel>::setToEnd() {
  restIterator_ = ASTNodeIteratorImpl<onlyFirstLevel>::CreateRestInstance(
      (*this->rootStmtIt_)->getChildren().end());
  return *this;
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
Stmt& SubtreeIteratorImpl<onlyFirstLevel>::operator*() const {
  if(isVisitingRoot())
    return **this->rootStmtIt_;
  return **restIterator_;
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<
    ASTNodeIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>>
SubtreeIteratorImpl<onlyFirstLevel>::toggleOnlyFirstLevelVisiting() const {
  auto* output = new SubtreeIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(
      !static_cast<bool>(onlyFirstLevel))>(this->rootStmtIt_, true);
  output->restIterator_ =
      restIterator_ == nullptr
          ? nullptr
          : SubtreeIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(
                onlyFirstLevel))>::CreateRestInstance(restIterator_->rootStmtIt_);
  return std::unique_ptr<ASTNodeIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(
      !static_cast<bool>(onlyFirstLevel))>>(output);
}

//
//  Definition of VoidIteratorImpl
//

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>
VoidIteratorImpl<onlyFirstLevel>::clone() const {
  VoidIteratorImpl* o = new VoidIteratorImpl(this->rootStmtIt_, this->isTop_);
  o->isEnd_ = isEnd_;
  return std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>>(o);
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<ASTNodeIteratorImpl<onlyFirstLevel>> VoidIteratorImpl<onlyFirstLevel>::
operator++() {
  isEnd_ = true;
  return this->clone();
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
bool VoidIteratorImpl<onlyFirstLevel>::
operator==(const ASTNodeIteratorImpl<onlyFirstLevel>& other) const {
  if(!other.isVoidIter())
    return false;
  return (*this == *dynamic_cast<VoidIteratorImpl const*>(&other));
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
bool VoidIteratorImpl<onlyFirstLevel>::operator==(const VoidIteratorImpl& other) const {
  return this->isTop() == other.isTop_ && this->rootStmtIt_ == other.rootStmtIt_ &&
         isEnd_ == other.isEnd_;
}

template <ASTNodeIteratorVisitKind onlyFirstLevel>
std::unique_ptr<
    ASTNodeIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>>
VoidIteratorImpl<onlyFirstLevel>::toggleOnlyFirstLevelVisiting() const {
  auto* output = new VoidIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(
      !static_cast<bool>(onlyFirstLevel))>(this->rootStmtIt_, true);
  output->isEnd_ = isEnd_;
  return std::unique_ptr<ASTNodeIteratorImpl<static_cast<ASTNodeIteratorVisitKind>(
      !static_cast<bool>(onlyFirstLevel))>>(output);
}

} // namespace dawn

#endif
