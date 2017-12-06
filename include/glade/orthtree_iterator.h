#ifndef __GLADE_ORTHTREE_ITERATOR_H_
#define __GLADE_ORTHTREE_ITERATOR_H_

#include <cstddef>
#include <iterator>
#include <type_traits>

#include "orthtree.h"

namespace glade {

/**
 * \brief An iterator over the LeafRangeBase container.
 * 
 * The Leaf%s are iterated over in depth-first order.
 * 
 * This iterator meets the requirements of `InputIterator`. It would also
 * the requirements of `RandomAccessIterator` except that the reference type
 * used by this iterator is not `Leaf&`. A proxy reference type
 * LeafReferenceProxyBase is used instead.
 * 
 * \tparam Const whether this is a `const` variant of the iterator
 * \tparam Reverse whether this is a reverse or forward iterator
 */
template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<bool Const, bool Reverse>
class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
LeafIteratorBase final {
	
private:
	
	friend Orthtree<Dim, Vector, LeafValue, NodeValue, Details>;
	
	template<typename>
	friend class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
		LeafRangeBase;
	
	using ReferenceProxy = LeafReferenceProxyBase<Const>;
	using List = LeafList;
	using ListSizeType = LeafListSizeType;
	using ListDifferenceType = LeafListDifferenceType;
	using ListIterator = std::conditional_t<
			Const,
			typename LeafList::const_iterator,
			typename LeafList::iterator>;
	using ListReference = std::conditional_t<
			Const,
			typename LeafList::const_reference,
			typename LeafList::reference>;
	using OrthtreePointer = std::conditional_t<
			Const,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details> const*,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details>*>;
	
	// This type is used by the arrow (->) operator.
	struct PointerProxy;
	
	OrthtreePointer _orthtree;
	ListDifferenceType _index;
	
	LeafIteratorBase(
			OrthtreePointer orthtree,
			ListDifferenceType index) :
			_orthtree(orthtree),
			_index(index) {
	}
	
	// Converts this iterator to an iterator over the internal leaf type.
	ListIterator internalIt() const {
		return _orthtree->_leafs.begin() + _index;
	}
	
public:
	
	// Iterator typedefs.
	using value_type = Leaf;
	using reference = ReferenceProxy;
	using pointer = PointerProxy;
	using size_type = ListSizeType;
	using difference_type = ListDifferenceType;
	using iterator_category = std::input_iterator_tag;
	
	LeafIteratorBase() :
			_orthtree(NULL),
			_index(0) {
	}
	
	operator LeafIteratorBase<true, Reverse>() const {
		return LeafIteratorBase<true, Reverse>(_orthtree, _index);
	}
	
	/**
	 * \brief Flips the direction of the iterator.
	 * 
	 * This method transforms a forward iterator into a reverse iterator, and a
	 * reverse iterator into a forward iterator. Note that the resulting
	 * does not point to the same value as the original iterator (same behaviour
	 * as `std::reverse_iterator<...>::base()`).
	 */
	LeafIteratorBase<Const, !Reverse> reverse() const {
		difference_type shift = !Reverse ? -1 : +1;
		return LeafIteratorBase<Const, !Reverse>(_orthtree, _index + shift);
	}
	
	// Iterator element access methods.
	reference operator*() const;
	pointer operator->() const;
	
	reference operator[](size_type n) const {
		return *(*this + n);
	}
	
	// Iterator increment methods.
	LeafIteratorBase<Const, Reverse>& operator++() {
		difference_type shift = Reverse ? -1 : +1;
		_index += shift;
		return *this;
	}
	LeafIteratorBase<Const, Reverse> operator++(int) {
		LeafIteratorBase<Const, Reverse> result = *this;
		operator++();
		return result;
	}
	
	LeafIteratorBase<Const, Reverse>& operator--() {
		difference_type shift = Reverse ? +1 : -1;
		_index += shift;
		return *this;
	}
	LeafIteratorBase<Const, Reverse> operator--(int) {
		LeafIteratorBase<Const, Reverse> result = *this;
		operator--();
		return result;
	}
	
	// Iterator arithmetic methods.
	LeafIteratorBase<Const, Reverse>& operator+=(difference_type n) {
		difference_type shift = Reverse ? -n : +n;
		_index += shift;
		return *this;
	}
	LeafIteratorBase<Const, Reverse>& operator-=(difference_type n) {
		difference_type shift = Reverse ? +n : -n;
		_index += shift;
		return *this;
	}
	
	friend LeafIteratorBase<Const, Reverse> operator+(
			LeafIteratorBase<Const, Reverse> it,
			difference_type n) {
		it += n;
		return it;
	}
	friend LeafIteratorBase<Const, Reverse> operator+(
			difference_type n,
			LeafIteratorBase<Const, Reverse> it) {
		return it + n;
	}
	friend LeafIteratorBase<Const, Reverse> operator-(
			LeafIteratorBase<Const, Reverse> it,
			difference_type n) {
		it -= n;
		return it;
	}
	friend difference_type operator-(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		difference_type diff = lhs._index - rhs._index;
		return Reverse ? -diff : +diff;
	}
	
	// Iterator comparison methods.
	friend bool operator<(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		return 0 < (rhs - lhs);
	}
	friend bool operator<=(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		return !(lhs > rhs);
	}
	friend bool operator>(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		return rhs < lhs;
	}
	friend bool operator>=(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		return !(lhs < rhs);
	}
	friend bool operator==(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		return lhs._index == rhs._index;
	}
	friend bool operator!=(
			LeafIteratorBase<Const, Reverse> const& lhs,
			LeafIteratorBase<Const, Reverse> const& rhs) {
		return !(lhs == rhs);
	}
	
};



/**
 * \brief An iterator over the NodeRangeBase container.
 * 
 * The Node%s are iterated over in depth-first order.
 * 
 * This iterator meets the requirements of `InputIterator`. It would also
 * the requirements of `BidirectionalIterator` except that the reference type
 * used by this iterator is not `Node&`. A proxy reference type
 * NodeReferenceProxyBase is used instead.
 * 
 * \tparam Const whether this is a `const` variant of the iterator
 * \tparam Reverse whether this is a reverse or forward iterator
 */
template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<bool Const, bool Reverse>
class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
NodeIteratorBase final {
	
private:
	
	friend Orthtree<Dim, Vector, LeafValue, NodeValue, Details>;
	
	template<typename>
	friend class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
		NodeRangeBase;
	
	using ReferenceProxy = NodeReferenceProxyBase<Const>;
	using List = NodeList;
	using ListSizeType = NodeListSizeType;
	using ListDifferenceType = NodeListDifferenceType;
	using ListIterator = std::conditional_t<
			Const,
			typename NodeList::const_iterator,
			typename NodeList::iterator>;
	using ListReference = std::conditional_t<
			Const,
			typename NodeList::const_reference,
			typename NodeList::reference>;
	using OrthtreePointer = std::conditional_t<
			Const,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details> const*,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details>*>;
	
	// This type is used by the arrow (->) operator.
	struct PointerProxy;
	
	OrthtreePointer _orthtree;
	ListDifferenceType _index;
	
	NodeIteratorBase(
			OrthtreePointer orthtree,
			ListDifferenceType index) :
			_orthtree(orthtree),
			_index(index) {
	}
	
	// Converts this iterator to an iterator over the internal node type.
	ListIterator internalIt() const {
		return _orthtree->_nodes.begin() + _index;
	}
	
public:
	
	// Iterator typedefs.
	using value_type = Node;
	using reference = ReferenceProxy;
	using pointer = PointerProxy;
	using size_type = ListSizeType;
	using difference_type = ListDifferenceType;
	using iterator_category = std::input_iterator_tag;
	
	NodeIteratorBase() :
			_orthtree(NULL),
			_index(0) {
	}
	
	operator NodeIteratorBase<true, Reverse>() const {
		return NodeIteratorBase<true, Reverse>(_orthtree, _index);
	}
	
	/**
	 * \brief Flips the direction of the iterator.
	 * 
	 * This method transforms a forward iterator into a reverse iterator, and a
	 * reverse iterator into a forward iterator. Note that the resulting
	 * does not point to the same value as the original iterator (same behaviour
	 * as `std::reverse_iterator<...>::base()`).
	 */
	NodeIteratorBase<Const, !Reverse> reverse() const {
		difference_type shift = !Reverse ? -1 : +1;
		return NodeIteratorBase<Const, !Reverse>(_orthtree, _index + shift);
	}
	
	// Iterator element access methods.
	reference operator*() const;
	pointer operator->() const;
	
	reference operator[](size_type n) const {
		return *(*this + n);
	}
	
	// Iterator increment methods.
	NodeIteratorBase<Const, Reverse>& operator++() {
		difference_type shift = Reverse ? -1 : +1;
		_index += shift;
		return *this;
	}
	NodeIteratorBase<Const, Reverse> operator++(int) {
		NodeIteratorBase<Const, Reverse> result = *this;
		operator++();
		return result;
	}
	
	NodeIteratorBase<Const, Reverse>& operator--() {
		difference_type shift = Reverse ? +1 : -1;
		_index += shift;
		return *this;
	}
	NodeIteratorBase<Const, Reverse> operator--(int) {
		NodeIteratorBase<Const, Reverse> result = *this;
		operator--();
		return result;
	}
	
	// Iterator arithmetic methods.
	NodeIteratorBase<Const, Reverse>& operator+=(difference_type n) {
		difference_type shift = Reverse ? -n : +n;
		_index += shift;
		return *this;
	}
	NodeIteratorBase<Const, Reverse>& operator-=(difference_type n) {
		difference_type shift = Reverse ? +n : -n;
		_index += shift;
		return *this;
	}
	
	friend NodeIteratorBase<Const, Reverse> operator+(
			NodeIteratorBase<Const, Reverse> it,
			difference_type n) {
		it += n;
		return it;
	}
	friend NodeIteratorBase<Const, Reverse> operator+(
			difference_type n,
			NodeIteratorBase<Const, Reverse> it) {
		return it + n;
	}
	friend NodeIteratorBase<Const, Reverse> operator-(
			NodeIteratorBase<Const, Reverse> it,
			difference_type n) {
		it -= n;
		return it;
	}
	friend difference_type operator-(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		difference_type diff = lhs._index - rhs._index;
		return Reverse ? -diff : +diff;
	}
	
	// Iterator comparison methods.
	friend bool operator<(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		return 0 < (rhs - lhs);
	}
	friend bool operator<=(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		return !(lhs > rhs);
	}
	friend bool operator>(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		return rhs < lhs;
	}
	friend bool operator>=(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		return !(lhs < rhs);
	}
	friend bool operator==(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		return lhs._index == rhs._index;
	}
	friend bool operator!=(
			NodeIteratorBase<Const, Reverse> const& lhs,
			NodeIteratorBase<Const, Reverse> const& rhs) {
		return !(lhs == rhs);
	}
	
};

}

#endif

#include "orthtree_iterator.tpp"

