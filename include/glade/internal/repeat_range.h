#ifndef __GLADE_INTERNAL_REPEAT_RANGE_H_
#define __GLADE_INTERNAL_REPEAT_RANGE_H_

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace glade {
namespace internal {

template<typename T, bool Const>
class RepeatRangeBase;

template<typename T, bool Const, bool Reverse>
class RepeatIteratorBase;

template<typename T>
using RepeatRange = RepeatRangeBase<T, false>;
template<typename T>
using ConstRepeatRange = RepeatRangeBase<T, true>;

template<typename T>
using RepeatIterator = RepeatIteratorBase<T, false, false>;
template<typename T>
using ConstRepeatIterator = RepeatIteratorBase<T, true, false>;
template<typename T>
using ReverseRepeatIterator = RepeatIteratorBase<T, false, true>;
template<typename T>
using ConstReverseRepeatIterator = RepeatIteratorBase<T, true, true>;

/**
 * \brief A container type for a range containing a single element a certain
 * number of times.
 */
template<typename T, bool Const>
class RepeatRangeBase final {
	
private:
	
	T _value;
	std::size_t _repetitions;
	
public:
	
	// Container typedefs.
	using iterator = std::conditional_t<Const,
		ConstRepeatIterator<T>,
		RepeatIterator<T> >;
	using const_iterator = ConstRepeatIterator<T>;
	using reverse_iterator = std::conditional_t<Const,
		ConstReverseRepeatIterator<T>,
		ReverseRepeatIterator<T> >;
	using const_reverse_iterator = ConstReverseRepeatIterator<T>;
	
	using value_type = typename iterator::value_type;
	using reference = typename iterator::reference;
	using const_reference = typename const_iterator::reference;
	using pointer = typename iterator::pointer;
	using const_pointer = typename const_iterator::pointer;
	using size_type = typename iterator::size_type;
	using difference_type = typename iterator::difference_type;
	
	RepeatRangeBase(T value, std::size_t repetitions) :
			_value(value),
			_repetitions(repetitions) {
	}
	
	operator RepeatRangeBase<T, true>() const {
		return RepeatRangeBase<T, true>(_value, _repetitions);
	}
	
	// Container iteration range methods.
	iterator begin() const {
		return iterator(&_value, 0);
	}
	const_iterator cbegin() const {
		return const_iterator(&_value, 0);
	}
	
	iterator end() const {
		return iterator(&_value, _repetitions);
	}
	const_iterator cend() const {
		return const_iterator(&_value, _repetitions);
	}
	
	reverse_iterator rbegin() const {
		return reverse_iterator(&_value, -1);
	}
	const_reverse_iterator crbegin() const {
		return const_reverse_iterator(&_value, -1);
	}
	
	reverse_iterator rend() const {
		return reverse_iterator(
			&_value,
			static_cast<difference_type>(_repetitions) - 1);
	}
	const_reverse_iterator crend() const {
		return const_reverse_iterator(
			&_value,
			static_cast<difference_type>(_repetitions) - 1);
	}
	
	// Element access methods.
	reference operator[](size_type index) {
		return begin()[index];
	}
	const_reference operator[](size_type index) const {
		return begin()[index];
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
		return _repetitions;
	}
	size_type max_size() const {
		return size();
	}
	bool empty() const {
		return _repetitions == 0;
	}
	
};

template<typename T, bool Const, bool Reverse>
class RepeatIteratorBase final {
	
private:
	
	using Pointer = std::conditional_t<Const, T const*, T*>;
	
	Pointer _value;
	std::ptrdiff_t _iteration;
	
	RepeatIteratorBase(Pointer value, std::ptrdiff_t iteration) :
			_value(value),
			_iteration(iteration) {
	}
	
public:
	
	// Iterator typedefs.
	using value_type = T;
	using reference = std::conditional_t<Const, T const&, T&>;
	using pointer = Pointer;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::random_access_iterator_tag;
	
	RepeatIteratorBase() :
			_value(NULL),
			_iteration(0) {
	}
	
	operator RepeatIteratorBase<T, true, Reverse>() const {
		return RepeatIteratorBase<T, true, Reverse>(_value, _iteration);
	}
	
	/**
	 * \brief Flips the direction of the iterator.
	 * 
	 * This method transforms a forward iterator into a reverse iterator, and a
	 * reverse iterator into a forward iterator. Note that the resulting
	 * does not point to the same value as the original iterator (same behaviour
	 * as `std::reverse_iterator<...>::base()`).
	 */
	RepeatIteratorBase<T, Const, !Reverse> reverse() const {
		difference_type shift = !Reverse ? -1 : +1;
		return RepeatIteratorBase<T, Const, !Reverse>(
			_value,
			_iteration + shift);
	}
	
	// Iterator element access methods.
	reference operator*() const {
		return *_value;
	}
	pointer operator->() const {
		return _value;
	}
	
	reference operator[](size_type n) const {
		return *(*this + n);
	}
	
	// Iterator increment methods.
	RepeatIteratorBase<T, Const, Reverse>& operator++() {
		difference_type shift = Reverse ? -1 : +1;
		_iteration += shift;
		return *this;
	}
	RepeatIteratorBase<T, Const, Reverse> operator++(int) {
		RepeatIteratorBase<T, Const, Reverse> result = *this;
		operator++();
		return result;
	}
	
	RepeatIteratorBase<T, Const, Reverse>& operator--() {
		difference_type shift = Reverse ? +1 : -1;
		_iteration += shift;
		return *this;
	}
	RepeatIteratorBase<T, Const, Reverse> operator--(int) {
		RepeatIteratorBase<T, Const, Reverse> result = *this;
		operator--();
		return result;
	}
	
	// Iterator arithmetic methods.
	RepeatIteratorBase<T, Const, Reverse>& operator+=(difference_type n) {
		difference_type shift = Reverse ? -n : +n;
		_iteration += shift;
		return *this;
	}
	RepeatIteratorBase<T, Const, Reverse>& operator-=(difference_type n) {
		difference_type shift = Reverse ? +n : -n;
		_iteration += shift;
		return *this;
	}
	
	friend RepeatIteratorBase<T, Const, Reverse> operator+(
			RepeatIteratorBase<T, Const, Reverse> it,
			difference_type n) {
		it += n;
		return it;
	}
	friend RepeatIteratorBase<T, Const, Reverse> operator+(
			difference_type n,
			RepeatIteratorBase<T, Const, Reverse> it) {
		return it + n;
	}
	friend RepeatIteratorBase<T, Const, Reverse> operator-(
			RepeatIteratorBase<T, Const, Reverse> it,
			difference_type n) {
		it -= n;
		return it;
	}
	friend difference_type operator-(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		difference_type diff = lhs._iteration - rhs._iteration;
		return Reverse ? -diff : +diff;
	}
	
	// Iterator comparison methods.
	friend bool operator<(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		return 0 < (rhs - lhs);
	}
	friend bool operator<=(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		return !(lhs > rhs);
	}
	friend bool operator>(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		return rhs < lhs;
	}
	friend bool operator>=(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		return !(lhs < rhs);
	}
	friend bool operator==(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		return lhs._iteration == rhs._iteration;
	}
	friend bool operator!=(
			RepeatIteratorBase<T, Const, Reverse> const& lhs,
			RepeatIteratorBase<T, Const, Reverse> const& rhs) {
		return !(lhs == rhs);
	}
};

}
}

#endif

