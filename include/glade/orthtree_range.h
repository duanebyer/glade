#ifndef __GLADE_ORTHTREE_RANGE_H_
#define __GLADE_ORTHTREE_RANGE_H_

#include <cstddef>
#include <type_traits>

#include "orthtree.h"

#include "orthtree_iterator.h"

namespace glade {

/**
 * \brief A pseudo-container that provides access to a collection of leaves
 * from an Orthtree.
 * 
 * The leaves are stored in depth-first order.
 * 
 * This container partially meets the requirements of `ReversibleContainer`. The
 * differences arise because a range cannot be created, and which elements it
 * contains cannot be changed. A range must be obtained from the Orthtree class.
 * 
 * \tparam Const whether this container allows for modifying its elements
 */
template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<bool Const>
class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
LeafRangeBase final {
	
private:
	
	friend Orthtree<Dim, Vector, LeafValue, NodeValue, Details>;
	
	template<typename>
	friend class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
		NodeIteratorBase;
	
	using OrthtreePointer = std::conditional_t<
			Const,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details> const*,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details>*>;
	
	OrthtreePointer _orthtree;
	LeafListSizeType _lowerIndex;
	LeafListSizeType _upperIndex;
	
	LeafRangeBase(
			OrthtreePointer orthtree,
			LeafListSizeType lowerIndex,
			LeafListSizeType upperIndex) :
			_orthtree(orthtree),
			_lowerIndex(lowerIndex),
			_upperIndex(upperIndex) {
	}
	
public:
	
	// Container typedefs.
	using iterator = std::conditional_t<Const,
		ConstLeafIterator,
		LeafIterator>;
	using const_iterator = ConstLeafIterator;
	using reverse_iterator = std::conditional_t<Const,
		ConstReverseLeafIterator,
		ReverseLeafIterator>;
	using const_reverse_iterator = ConstReverseLeafIterator;
	
	using value_type = typename iterator::value_type;
	using reference = typename iterator::reference;
	using const_reference = typename const_iterator::reference;
	using pointer = typename iterator::pointer;
	using const_pointer = typename const_iterator::pointer;
	using size_type = typename iterator::size_type;
	using difference_type = typename iterator::difference_type;
	
	operator LeafRangeBase<true>() const {
		return LeafRangeBase<true>(_orthtree, _lowerIndex, _upperIndex);
	}
	
	/**
	 * \brief Provides read-only access to the raw memory where the data for
	 * this range is stored.
	 */
	Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafInternal const*
	data() const {
		return _orthtree->_leafs.data() + _lowerIndex;
	}
	
	// Container iteration range methods.
	iterator begin() const {
		return iterator(_orthtree, _lowerIndex);
	}
	const_iterator cbegin() const {
		return const_iterator(_orthtree, _lowerIndex);
	}
	
	iterator end() const {
		return iterator(_orthtree, _upperIndex);
	}
	const_iterator cend() const {
		return const_iterator(_orthtree, _upperIndex);
	}
	
	reverse_iterator rbegin() const {
		return reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_upperIndex) - 1);
	}
	const_reverse_iterator crbegin() const {
		return const_reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_upperIndex) - 1);
	}
	
	reverse_iterator rend() const {
		return reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_lowerIndex) - 1);
	}
	const_reverse_iterator crend() const {
		return const_reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_lowerIndex) - 1);
	}
	
	// Element access methods.
	reference operator[](size_type index) {
		return *(begin() + index);
	}
	const_reference operator[](size_type index) const {
		return *(begin() + index);
	}
	reference front() {
		return *begin();
	}
	const_reference front() const {
		return *begin();
	}
	reference back() {
		return *(end() - 1);
	}
	const_reference back() const {
		return *(end() - 1);
	}
	
	// Container size methods.
	size_type size() const {
		return _upperIndex - _lowerIndex;
	}
	size_type max_size() const {
		return size();
	}
	bool empty() const {
		return _upperIndex == _lowerIndex;
	}
	
};



/**
 * \brief A pseudo-container that provides access to a collection of nodes
 * from an Orthtree.
 * 
 * The nodes are stored in depth-first order.
 * 
 * This container partially meets the requirements of `ReversibleContainer`. The
 * differences arise because a range cannot be created, and which elements it
 * contains cannot be changed. A range must be obtained from the Orthtree class.
 * 
 * \tparam Const whether this container allows for modifying its elements
 */
template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<bool Const>
class Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
NodeRangeBase final {
	
private:
	
	friend Orthtree;
	
	using OrthtreePointer = std::conditional_t<
			Const,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details> const*,
			Orthtree<Dim, Vector, LeafValue, NodeValue, Details>*>;
	
	OrthtreePointer _orthtree;
	NodeListSizeType _lowerIndex;
	NodeListSizeType _upperIndex;
	
	NodeRangeBase(
			OrthtreePointer orthtree,
			NodeListSizeType lowerIndex,
			NodeListSizeType upperIndex) :
			_orthtree(orthtree),
			_lowerIndex(lowerIndex),
			_upperIndex(upperIndex) {
	}
	
public:
	
	// Container typedefs.
	using iterator = std::conditional_t<Const,
		ConstNodeIterator,
		NodeIterator>;
	using const_iterator = ConstNodeIterator;
	using reverse_iterator = std::conditional_t<Const,
		ConstReverseNodeIterator,
		ReverseNodeIterator>;
	using const_reverse_iterator = ConstReverseNodeIterator;
	
	using value_type = typename iterator::value_type;
	using reference = typename iterator::reference;
	using const_reference = typename const_iterator::reference;
	using pointer = typename iterator::pointer;
	using const_pointer = typename const_iterator::pointer;
	using size_type = typename iterator::size_type;
	using difference_type = typename iterator::difference_type;
	
	operator NodeRangeBase<true>() const {
		return NodeRangeBase<true>(_orthtree, _lowerIndex, _upperIndex);
	}
	
	/**
	 * \brief Provides read-only access to the raw memory where the data for
	 * this range is stored.
	 */
	Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeInternal const*
	data() const {
		return _orthtree->_nodes.data() + _lowerIndex;
	}
	
	// Container iteration range methods.
	iterator begin() const {
		return iterator(_orthtree, _lowerIndex);
	}
	const_iterator cbegin() const {
		return const_iterator(_orthtree, _lowerIndex);
	}
	
	iterator end() const {
		return iterator(_orthtree, _upperIndex);
	}
	const_iterator cend() const {
		return const_iterator(_orthtree, _upperIndex);
	}
	
	reverse_iterator rbegin() const {
		return reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_upperIndex) - 1);
	}
	const_reverse_iterator crbegin() const {
		return const_reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_upperIndex) - 1);
	}
	
	reverse_iterator rend() const {
		return reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_lowerIndex) - 1);
	}
	const_reverse_iterator crend() const {
		return const_reverse_iterator(
			_orthtree,
			static_cast<difference_type>(_lowerIndex) - 1);
	}
	
	// Element access methods.
	reference operator[](size_type index) {
		return *(begin() + index);
	}
	const_reference operator[](size_type index) const {
		return *(begin() + index);
	}
	reference front() {
		return *begin();
	}
	const_reference front() const {
		return *begin();
	}
	reference back() {
		return *(end() - 1);
	}
	const_reference back() const {
		return *(end() - 1);
	}
	
	// Container size methods.
	size_type size() const {
		return _upperIndex - _lowerIndex;
	}
	size_type max_size() const {
		return size();
	}
	bool empty() const {
		return _upperIndex == _lowerIndex;
	}
	
};

}

#endif

