#ifndef __GLADE_ORTHTREE_INTERNAL_DETAILS_DEFAULT_H_
#define __GLADE_ORTHTREE_INTERNAL_DETAULS_DEFAULT_H_

namespace glade {

/**
 * \brief A collection of types that determine the implementation details of the
 * Orthtree class.
 * 
 * For instance, this class can be used to change the vector type used by the
 * Orthtree class to store nodes and leafs, or to change the size type. This
 * class can be extended to create custom implementation schemes for Orthtree.
 * 
 * This class is exposed because Orthtree exposes a read-only interface to
 * internal data for high performance situations. In such situations, it may be
 * important to control the implementation of the Orthtree class.
 */
struct OrthtreeInternalDetailsDefault {
	
	/**
	 * \brief A list type that provides similar functionality to std::vector.
	 */
	template<typename T>
	using VectorType = std::vector<T>;
	
	/**
	 * \brief An unsigned type that is used to index instances of VectorType.
	 */
	template<typename T>
	using SizeType = typename VectorType<T>::size_type;
	
	/**
	 * \brief A signed version of SizeType.
	 */
	template<typename T>
	using DifferenceType = typename VectorType<T>::difference_type;
	
};

}

#endif

