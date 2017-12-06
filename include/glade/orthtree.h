#ifndef __GLADE_ORTHTREE_H_
#define __GLADE_ORTHTREE_H_

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "orthtree_internal_details_default.h"

#include "internal/functional.h"
#include "internal/repeat_range.h"
#include "internal/type_traits.h"

namespace glade {

/**
 * \brief A data structure that stores spatial data in arbitrary dimensional
 * space.
 * 
 * An orthtree is the extension of a quadtree/octree to arbitrary dimensional
 * space. This class implements an orthtree that stores data at discrete points
 * (the leaves) as well as at the nodes of the underlying tree structure.
 * 
 * The Orthtree class is designed to be compatible with the STL container
 * library. It acts as two different kinds of containers: one container of leaf
 * data (see the LeafRangeBase class), and another container of nodes (see the
 * NodeRangeBase class). These containers can be accessed using the
 * Orthtree::leafs and Orthtree::nodes methods.
 * 
 * Data can be added to or removed from the Orthtree through the
 * Orthtree::insert, Orthtree::erase, and Orthtree::move methods.
 * 
 * One of the advantages of this implementation is that all node and leaf data
 * is stored contigiously in memory. For certain applications, this is more
 * useful than the conventional implementation of an orthtree where nodes
 * contain pointers to other nodes scattered throughout memory. In general, this
 * means that most operations perform slightly worse than usual when using this
 * implementation.
 * 
 * \tparam Dim the dimension of the space that the Orthtree is embedded in
 * \tparam Vector a `Dim`-dimensional vector type that supports `operator[]`
 * \tparam LeafValue the type of data stored at the leaves of the Orthtree
 * \tparam NodeValue the type of data stored at the nodes of the Orthtree
 * \tparam OctreeInternalDetails type that stores implementation details
 */
template<
	std::size_t Dim,
	typename Vector = std::array<double, Dim>,
	typename LeafValue = std::tuple<>,
	typename NodeValue = std::tuple<>,
	typename Details = OrthtreeInternalDetailsDefault >
class Orthtree final {
	
public:
	
	// Make sure that the template parameters meet all of the conditions.
	
	using Scalar = std::remove_reference_t<decltype(std::declval<Vector>()[0])>;
	
	static_assert(
		Dim > 0,
		"template parameter Dim must be larger than 0");
	
	static_assert(
		std::is_default_constructible<NodeValue>::value,
		"template parameter NodeValue must be default constructible");
	static_assert(
		std::is_copy_assignable<NodeValue>::value,
		"template parameter NodeValue must be copy assignable");
	static_assert(
		std::is_copy_constructible<NodeValue>::value,
		"template parameter NodeValue must be copy constructible");
	
	static_assert(
		std::is_default_constructible<LeafValue>::value,
		"template parameter LeafValue must be default constructible");
	static_assert(
		std::is_copy_assignable<LeafValue>::value,
		"template parameter LeafValue must be copy assignable");
	static_assert(
		std::is_copy_constructible<LeafValue>::value,
		"template parameter LeafValue must be copy constructible");
	
	static_assert(
		std::is_copy_constructible<Vector>::value,
		"template parameter Vector must be copy constructible");
	static_assert(
		glade::internal::is_invocable<
			glade::internal::subscript<Vector>,
			Vector,
			std::size_t>::value,
		"template parameter Vector must have operator[]");
	static_assert(
		glade::internal::is_invocable_r<
			Scalar&, glade::internal::subscript<Vector>,
			Vector,
			std::size_t>::value,
		"template parameter Vector must have operator[]");
	
	static_assert(
		glade::internal::is_invocable_r<
			Scalar, std::plus<Scalar>, Scalar, Scalar>::value,
		"template parameter Vector's scalar type must have operator+");
	static_assert(
		glade::internal::is_invocable_r<
			Scalar, std::minus<Scalar>, Scalar, Scalar>::value,
		"template parameter Vector's scalar type must have operator-");
	static_assert(
		glade::internal::is_invocable_r<
			Scalar, std::multiplies<Scalar>, Scalar, Scalar>::value,
		"template parameter Vector's scalar type must have operator*");
	static_assert(
		glade::internal::is_invocable_r<
			Scalar, std::multiplies<Scalar>, Scalar, Scalar>::value,
		"template parameter Vector's scalar type must have operator/");
	static_assert(
		glade::internal::is_invocable_r<
			Scalar, std::less<Scalar>, Scalar, Scalar>::value,
		"template parameter Vector's scalar type must have operator<");
	static_assert(
		glade::internal::is_invocable_r<
			Scalar, std::greater_equal<Scalar>, Scalar, Scalar>::value,
		"template parameter Vector's scalar type must have operator>=");
	
	// These classes are used as proxies for accessing the nodes and leafs of
	// the orthtree.
	
	struct Leaf;
	
	template<bool Const>
	struct LeafReferenceProxyBase;
	
	template<bool Const>
	struct NodeBase;
	
	template<bool Const>
	struct NodeReferenceProxyBase;
	
	///@{
	/**
	 * \brief Provides access to a node of the Orthtree.
	 * 
	 * These types are `const` and non-`const` specializations of the NodeBase
	 * class, since NodeBase allows for traversal of the Orthtree through
	 * iterators. Otherwise, it might be possible to obtain a non-`const`
	 * reference from a `const` iterator.
	 */
	using Node = NodeBase<false>;
	using ConstNode = NodeBase<true>;
	///@}
	
	///@{
	/**
	 * \brief Proxy type that acts as a reference to a Leaf.
	 * 
	 * \see LeafReferenceProxyBase
	 */
	using LeafReferenceProxy = LeafReferenceProxyBase<false>;
	using ConstLeafReferenceProxy = LeafReferenceProxyBase<true>;
	///@}
	
	///@{
	/**
	 * \brief Proxy type that acts as a reference to a Node.
	 * 
	 * \see NodeReferenceProxyBase
	 */
	using NodeReferenceProxy = NodeReferenceProxyBase<false>;
	using ConstNodeReferenceProxy = NodeReferenceProxyBase<true>;
	///@}
	
	// These classes are used to construct const-variants and reverse-variants
	// of the public range and iterator classes.
	
	template<bool Const, bool Reverse>
	class LeafIteratorBase;	
	
	template<bool Const>
	class LeafRangeBase;
	
	template<bool Const, bool Reverse>
	class NodeIteratorBase;	
	
	template<bool Const>
	class NodeRangeBase;
	
	///@{
	/**
	 * \brief Depth-first iterator over all of the leaves contained in the
	 * Orthtree.
	 * 
	 * \see LeafIteratorBase
	 */
	using LeafIterator = LeafIteratorBase<false, false>;
	using ConstLeafIterator = LeafIteratorBase<true, false>;
	using ReverseLeafIterator = LeafIteratorBase<false, true>;
	using ConstReverseLeafIterator = LeafIteratorBase<true, true>;
	///@}
	
	///@{
	/**
	 * \brief Pseudo-container that provides access to the leaves of the
	 * Orthtree without allowing for insertion or deletion of elements.
	 * 
	 * \see LeafRangeBase
	 */
	using LeafRange = LeafRangeBase<false>;
	using ConstLeafRange = LeafRangeBase<true>;
	///@}
	
	///@{
	/**
	 * \brief Depth-first iterator over all of the nodes contained in the
	 * Orthtree.
	 * 
	 * \see NodeIteratorBase
	 */
	using NodeIterator = NodeIteratorBase<false, false>;
	using ConstNodeIterator = NodeIteratorBase<true, false>;
	using ReverseNodeIterator = NodeIteratorBase<false, true>;
	using ConstReverseNodeIterator = NodeIteratorBase<true, true>;
	///@}
	
	///@{
	/**
	 * \brief Pseudo-container that provides access to the nodes of the Orthtree
	 * without allowing for insertion or deletion of elements.
	 * 
	 * \see NodeRangeBase
	 */
	using NodeRange = NodeRangeBase<false>;
	using ConstNodeRange = NodeRangeBase<true>;
	///@}
	
	struct LeafInternal;
	struct NodeInternal;
	
	using LeafList = typename Details::template VectorType<LeafInternal>;
	using NodeList = typename Details::template VectorType<NodeInternal>;
	using LeafListSizeType = typename Details::template SizeType<LeafInternal>;
	using NodeListSizeType = typename Details::template SizeType<NodeInternal>;
	using LeafListDifferenceType =
		typename Details::template DifferenceType<LeafInternal>;
	using NodeListDifferenceType =
		typename Details::template DifferenceType<NodeInternal>;
	
	/**
	 * \brief Packages together a position with leaf data.
	 * 
	 * This class is used internally to store leaf data. It should not be used
	 * normally. Use Orthtree::LeafIterator%s instead. It is exposed for cases
	 * in which direct access to the memory of the Orthtree is necessary.
	 */
	struct LeafInternal final {
		
		Vector position;
		LeafValue value;
		
		LeafInternal(Vector position, LeafValue value = LeafValue()) :
				position(position),
				value(value) {
		}
		
	};
	
	/**
	 * \brief Packages node data.
	 * 
	 * This class is used internally to store node data. It should not be used
	 * normally. Use Orthtree::NodeIterator%s instead. It is exposed for cases
	 * in which direct access to the memory of the Orthtree is necessary.
	 */
	struct NodeInternal final {
		
		// The section of space that this node encompasses.
		Vector position;
		Vector dimensions;
		
		// The depth of this node within the orthtree (0 for root, and so on).
		NodeListSizeType depth;
		
		// The indices of the children of this node, stored relative to the
		// index of this node. The last entry points to the next sibling of this
		// node, and is used to determine the total size of all of this node's
		// children.
		NodeListSizeType childIndices[(1 << Dim) + 1];
		
		// The relative index of the parent of this node.
		NodeListDifferenceType parentIndex;
		// Which child # of its parent this node is. (0th child, 1st child,
		// etc).
		NodeListSizeType siblingIndex;
		
		// The number of leaves that this node contains. This includes leaves
		// stored by all descendants of this node.
		LeafListSizeType leafCount;
		// The index within the orthtree's leaf array that this node's leaves
		// are located at.
		LeafListSizeType leafIndex;
		
		// Whether this node has any children.
		bool hasChildren;
		
		// The data stored at the node itself.
		NodeValue value;
		
		// By default, a node is constructed as if it were an empty root node.
		NodeInternal(
				Vector position,
				Vector dimensions,
				NodeValue value = NodeValue()) :
				position(position),
				dimensions(dimensions),
				depth(0),
				childIndices(),
				parentIndex(),
				siblingIndex(),
				leafCount(0),
				leafIndex(0),
				hasChildren(false),
				value(value) {
			std::fill(childIndices, childIndices + (1 << Dim) + 1, 1);
		}
		
	};
	
private:
	
	// A list storing all of the leafs of the orthtree.
	LeafList _leafs;
	
	// A list storing all of the nodes of the orthtree.
	NodeList _nodes;
	
	// The number of leaves to store at a single node of the orthtree.
	LeafListSizeType _nodeCapacity;
	
	// The maximum depth of the orthtree.
	NodeListSizeType _maxDepth;
	
	// Whether the tree should be automatically readjust itself so that each
	// node has less leaves than the node capacity, as well as having as few
	// children as possible. If this is false, then the adjust() method has to
	// be called to force an adjustment.
	bool _autoAdjust;
	
	
	
	// Determines whether a node can store a certain number of additional (or
	// fewer) leafs.
	bool canHoldLeafs(
			ConstNodeIterator node,
			LeafListDifferenceType n) const {
		return
			node->leafs.size() + n <= _nodeCapacity ||
			node->depth >= _maxDepth;
	}
	
	// Divides a node into a set of subnodes and partitions its leaves between
	// them. This function may reorganize the leaf vector (some leaf iterators
	// may become invalid).
	void createChildren(NodeIterator node);
	// Destroys all descendants of a node and takes their leaves into the node.
	// This function will not reorganize the leaf vector (all leaf iterators
	// will remain valid).
	void destroyChildren(NodeIterator node);
	
	// Allocates space for a new set of children for a node.
	void allocChildren(NodeIterator node);
	// Deallocates space for the children of a node.
	void freeChildren(NodeIterator node);
	
	// Updates a node and its ancestors' child indices after a node has children
	// created or destroyed. Returns the change in the number of children.
	NodeListDifferenceType updateNodeChildData(
		NodeIterator node,
		// Were children created (true) or destroyed (false) for the node.
		bool childrenCreated,
		// Should parent indices be updated (usually yes).
		bool updateParentIndices = true);
	
	// Updates only a node's ancestors after a node has children created or
	// destroyed. Returns the change in the number of children (same as its
	// second argument).
	NodeListDifferenceType updateAncestorChildData(
		NodeIterator node,
		NodeListDifferenceType childCountChange,
		bool updateParentIndices = true);
	
	// Distributes the leafs of a node to its children.
	void distributeLeafs(NodeIterator node);
	
	// Adds a leaf to a specific node.
	LeafIterator insertAt(
			NodeIterator node,
			LeafValue const& value,
			Vector const& position);
	
	// Removes a leaf from a node.
	LeafIterator eraseAt(
			NodeIterator node,
			LeafIterator leaf);
	
	// Moves a leaf from this node to another one.
	LeafIterator moveAt(
			NodeIterator sourceNode,
			NodeIterator destNode,
			LeafIterator sourceLeaf);
	
public:
	
	///@{
	/**
	 * \brief Constructs a new, empty Orthtree.
	 * 
	 * \param position { the location of the "upper-left" corner of the region
	 * of space that the Orthtree covers }
	 * \param dimensions the size of the region of space that the Orthtree
	 * covers
	 * \leafBegin the start of a range of leafs to insert
	 * \leafEnd the end of a range of leafs to insert
	 * \positionBegin the start of a range of positions to insert at
	 * \positionEnd the end of a range of positions to insert at
	 * \param nodeCapacity { the number of leaves that can be stored at
	 * one node }
	 * \param maxDepth the maximum number of generations of nodes
	 * \param adjust { whether the Orthtree should automatically create and
	 * destroy nodes to optimize the number of leaves per node }
	 */
	Orthtree(
		Vector position,
		Vector dimensions,
		LeafListSizeType nodeCapacity = 1,
		NodeListSizeType maxDepth = sizeof(Scalar) * CHAR_BIT,
		bool autoAdjust = true);
	
	template<typename LeafIt, typename PositionIt>
	Orthtree(
		Vector position,
		Vector dimensions,
		LeafIt leafBegin,
		LeafIt leafEnd,
		PositionIt positionBegin,
		PositionIt positionEnd,
		LeafListSizeType nodeCapacity = 1,
		NodeListSizeType maxDepth = sizeof(Scalar) * CHAR_BIT,
		bool autoAdjust = true);
	///@}
	
	LeafListSizeType nodeCapacity() const {
		return _nodeCapacity;
	}
	NodeListSizeType maxDepth() const {
		return _maxDepth;
	}
	bool autoAdjust() const {
		return _autoAdjust;
	}
	void autoAdjust(bool autoAdjust) {
		_autoAdjust = autoAdjust;
	}
	
	/**
	 * \brief Reserves approximately the amount of space needed for a certain
	 * number of leaves.
	 */
	void reserve(LeafListSizeType count) {
		// This relationship has been measured emperically with uniformly
		// distributed particles in an octree with node capacity of 1.
		double nodesCount = (3.8 * count + 400) * (1 << Dim) / 8.0;
		_leafs.reserve(count);
		// Add a factor of two for padding. This should handle the majority of
		// distributions.
		_nodes.reserve(static_cast<NodeListSizeType>(2.0 * nodesCount));
	}
	
	///@{
	/**
	 * \brief Gets a range that contains the leaves of the Orthtree.
	 */
	LeafRange leafs();
	ConstLeafRange leafs() const;
	ConstLeafRange cleafs() const;
	///@}
	
	///@{
	/**
	 * \brief Gets a range that contains the nodes of the Orthtree.
	 */
	NodeRange nodes();
	ConstNodeRange nodes() const;
	ConstNodeRange cnodes() const;
	///@}
	
	///@{
	/**
	 * \brief Gets an iterator to the root node of the Orthtree.
	 */
	NodeIterator root();
	ConstNodeIterator root() const;
	ConstNodeIterator croot() const;
	///@}
	
	///@{
	/**
	 * \brief Gets a range that contains all descendants of a specific node.
	 */
	NodeRange descendants(ConstNodeIterator node);
	ConstNodeRange descendants(ConstNodeIterator node) const;
	ConstNodeRange cdescendants(ConstNodeIterator node) const;
	///@}
	
	///@{
	/**
	 * \brief Creates and destroys nodes to optimize the number of leaves stored
	 * at each node.
	 * 
	 * This method will check for nodes that contain more than the maximum
	 * number of leaves, as well as for unnecessary nodes. The node structure
	 * of the Orthtree will be adjusted so that these situations are resolved.
	 * 
	 * If the Orthtree was constructed to automatically adjust itself, then this
	 * method will never do anything.
	 * 
	 * NodeIterator%s may be invalidated.
	 * 
	 * \param node an iterator to the node which will be adjusted
	 * 
	 * \return whether any changes were actually made
	 */
	bool adjust(NodeIterator node);
	bool adjust() {
		return adjust(root());
	}
	///@}
	
	///@{
	/**
	 * \brief Adds a new leaf to the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to insert the leaf at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leaf should be placed
	 * \param value the data to be inserted at the leaf
	 * \param position the position of the leaf
	 * 
	 * \return a tuple containing the NodeIterator to the node that the
	 * leaf was added to, and a LeafIterator to the new leaf
	 */
	std::tuple<NodeIterator, LeafIterator> insert(
			ConstNodeIterator hint,
			LeafValue value,
			Vector const& position);
	
	std::tuple<NodeIterator, LeafIterator> insert(
			LeafValue value,
			Vector const& position) {
		return insert(root(), value, position);
	}
	
	std::tuple<NodeIterator, LeafIterator> insert(
			ConstNodeIterator hint,
			Vector const& position) {
		return insert(hint, LeafValue(), position);
	}
	std::tuple<NodeIterator, LeafIterator> insert(
			Vector const& position) {
		return insert(root(), position);
	}
	///@}
	
	///@{
	/**
	 * \brief Adds a new leaf to the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to insert the leaf at the `hint` node.
	 * 
	 * The provided data should be packaged in a tuple. It will be accessed
	 * the `std::get` method.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leaf should be placed
	 * \param leafPair a tuple containing both the leaf and its position
	 * 
	 * \return a tuple containing the NodeIterator to the node that the
	 * leaf was added to, and a LeafIterator to the new leaf
	 */
	template<typename LeafTuple>
	std::tuple<NodeIterator, LeafIterator> insertTuple(
			ConstNodeIterator hint,
			LeafTuple leafPair) {
		return insert(
			hint,
			std::get<LeafValue>(leafPair),
			std::get<Vector>(leafPair));
	}
	
	template<typename LeafTuple>
	std::tuple<NodeIterator, LeafIterator> insertTuple(
			LeafTuple leafPair) {
		return insertTuple(root(), leafPair);
	}
	///@}
	
	///@{
	/**
	 * \brief Adds a range of new leafs to the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to insert the leaf at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leafs should be placed
	 * \param leafBegin the start of a range of leaf data
	 * \param leafEnd the end of a range of leaf data
	 * \param positionBegin the start of a range of position data
	 * \param positionEnd the end of a range of position data
	 */
	template<typename LeafIt, typename PositionIt>
	void insert(
		ConstNodeIterator hint,
		LeafIt leafBegin, LeafIt leafEnd,
		PositionIt positionBegin, PositionIt positionEnd);
	
	template<typename LeafIt, typename PositionIt>
	void insert(
			LeafIt leafBegin, LeafIt leafEnd,
			PositionIt positionBegin, PositionIt positionEnd) {
		insert(root(), leafBegin, leafEnd, positionBegin, positionEnd);
	}
	
	template<typename PositionIt>
	void insert(
			LeafValue value,
			PositionIt positionBegin, PositionIt positionEnd) {
		internal::RepeatRange<LeafValue> leafRange(
			value,
			std::distance(positionBegin, positionEnd));
		insert(leafRange.begin(), leafRange.end(), positionBegin, positionEnd);
	}
	
	template<typename PositionIt>
	void insert(PositionIt positionBegin, PositionIt positionEnd) {
		insert(LeafValue(), positionBegin, positionEnd);
	}
	///@}
	
	///@{
	/**
	 * \brief Adds a range of new leafs to the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to insert the leaf at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leafs should be placed
	 * \param leafPairBegin the start of a range of tuples of leaf-position data
	 * \param leafPairEnd the end of a range of tuples of leaf-position data
	 */
	template<typename LeafTupleIt>
	void insertTuple(
			ConstNodeIterator hint,
			LeafTupleIt leafPairBegin, LeafTupleIt leafPairEnd) {
		using LeafTuple = decltype(*leafPairBegin);
		LeafListSizeType size = std::distance(leafPairBegin, leafPairEnd);
		std::vector<LeafValue> leafValues;
		std::vector<Vector> positions;
		leafValues.reserve(size);
		positions.reserve(size);
		std::transform(
			leafPairBegin, leafPairEnd,
			std::back_inserter(leafValues),
			[](LeafTuple const& tuple) { return std::get<LeafValue>(tuple); });
		std::transform(
			leafPairBegin, leafPairEnd,
			std::back_inserter(positions),
			[](LeafTuple const& tuple) { return std::get<Vector>(tuple); });
		insert(
			hint,
			leafValues.begin(), leafValues.end(),
			positions.begin(), positions.end());
	}
	
	template<typename LeafTupleIt>
	void insertTuple(LeafTupleIt leafPairBegin, LeafTupleIt leafPairEnd) {
		insertTuple(root(), leafPairBegin, leafPairEnd);
	}
	///@}
	
	///@{
	/**
	 * \brief Removes an leaf from the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to remove the leaf from at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leaf should be removed from
	 * \param leaf a LeafIterator to the leaf that should be removed
	 * 
	 * \return a tuple containing the NodeIterator that the leaf was removed
	 * from, and the LeafIterator to the leaf after the removed leaf
	 */
	std::tuple<NodeIterator, LeafIterator> erase(
			ConstNodeIterator hint,
			LeafIterator leaf);
	
	std::tuple<NodeIterator, LeafIterator> erase(
			LeafIterator leaf) {
		return erase(root(), leaf);
	}
	///@}
	
	///@{
	/**
	 * \brief Removes a range of leafs from the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to remove the leaf from at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leafs should be removed from
	 * \param leafBegin the start of a range of leafs to be removed
	 * \param leafEnd the end of a range of leafs to be removed
	 */
	void erase(
		ConstNodeIterator hint,
		LeafIterator leafBegin, LeafIterator leafEnd);
	
	void erase(LeafIterator leafBegin, LeafIterator leafEnd) {
		erase(root(), leafBegin, leafEnd);
	}
	///@}
	
	///@{
	/**
	 * \brief Changes the position of a leaf within the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to move the leaf from at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leaf should be moved from
	 * \param leaf a LeafIterator to the leaf that should be moved
	 * \param position the new position that the leaf should be moved to
	 * 
	 * \return a tuple containing the NodeIterator that the leaf was removed
	 * from, the NodeIterator that it was moved to, and the LeafIterator itself
	 */
	std::tuple<NodeIterator, NodeIterator, LeafIterator> move(
			ConstNodeIterator hint,
			LeafIterator leaf,
			Vector const& position);
	
	std::tuple<NodeIterator, NodeIterator, LeafIterator> move(
			LeafIterator leaf,
			Vector const& position) {
		return move(root(), leaf, position);
	}
	///@}
	
	///@{
	/**
	 * \brief Changes the position of a range of leafs within the Orthtree.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node to move the leaf from at the `hint` node.
	 * 
	 * NodeIterator%s and LeafIterator%s may be invalidated.
	 * 
	 * \param hint a starting guess as to where the leaf should be moveds from
	 * \param leafBegin the start of a range of leafs to move
	 * \param leafEnd the end of a range of leafs to move
	 * \param positionBegin the start of a range of positions to move to
	 * \param positionEnd the end of a range of positions to move to
	 */
	template<typename PositionIt>
	void move(
		ConstNodeIterator hint,
		LeafIterator leafBegin, LeafIterator leafEnd,
		PositionIt positionBegin, PositionIt positionEnd);
	
	template<typename PositionIt>
	void move(
			LeafIterator leafBegin, LeafIterator leafEnd,
			PositionIt positionBegin, PositionIt positionEnd) {
		move(root(), leafBegin, leafEnd, positionBegin, positionEnd);
	}
	///@}
	
	///@{
	/**
	 * \brief Searches for the node that contains a certain position.
	 * 
	 * This method searches for the lowest-level node that contains a position.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node at the `hint` node.
	 * 
	 * \param hint an initial guess as to what node contains the position
	 * \param position the position to search for
	 * 
	 * \return the node that contains the position
	 */
	NodeIterator find(
			ConstNodeIterator hint,
			Vector const& point);
	
	NodeIterator find(
			Vector const& point) {
		return find(root(), point);
	}
	
	ConstNodeIterator find(
			ConstNodeIterator start,
			Vector const& point) const {
		return const_cast<std::remove_const_t<decltype(*this)*> >(this)->
			find(start, point);
	}
	
	ConstNodeIterator find(
			Vector const& point) const {
		return find(root(), point);
	}
	///@}
	
	///@{
	/**
	 * \brief Searchs for the node that contains a certain leaf.
	 * 
	 * This method searches for the lowest-level node that contains a leaf.
	 * 
	 * If the optional `hint` parameter is provided, then this method will begin
	 * its search for the node at the `hint` node.
	 * 
	 * \param start an initial guess as to what Node contains the position
	 * \param leaf the leaf to search for
	 * 
	 * \return the node that contains the leaf
	 */
	NodeIterator find(
			ConstNodeIterator hint,
			ConstLeafIterator leaf);
	
	NodeIterator find(
			ConstLeafIterator leaf) {
		return find(nodes().begin(), leaf);
	}
	
	ConstNodeIterator find(
			ConstNodeIterator hint,
			ConstLeafIterator leaf) const {
		return const_cast<std::remove_const_t<decltype(*this)*> >(this)->
			find(hint, leaf);
	}
	
	ConstNodeIterator find(
			ConstLeafIterator leaf) const {
		return find(nodes().begin(), leaf);
	}
	///@}
	
	///@{
	/**
	 * \brief Searches a node for one of its children that contains a certain
	 * position.
	 * 
	 * This method divides all of space into the octants of the node. Even if
	 * the position is not technically contained by one of the children, the
	 * child that would contain the position if it was extended to infinity will
	 * returned.
	 * 
	 * If the node has no children, then the result is undefined.
	 * 
	 * \param node the parent of the children that will be searched
	 * \param position the position to search for
	 */
	NodeIterator findChild(
			ConstNodeIterator node,
			Vector const& point);
	ConstNodeIterator findChild(
			ConstNodeIterator node,
			Vector const& point) const {
		return const_cast<std::remove_const_t<decltype(*this)*> >(this)->
			findChild(node, point);
	}
	///@}
	
	///@{
	/**
	 * \brief Searches a node for one of its children that contains a certain
	 * leaf.
	 * 
	 * If the node has no children, then the result is undefined.
	 * 
	 * \param node the parent of the children that will be searched
	 * \param leaf the leaf to search for
	 */
	NodeIterator findChild(
			ConstNodeIterator node,
			ConstLeafIterator leaf);
	ConstNodeIterator findChild(
			ConstNodeIterator node,
			ConstLeafIterator leaf) const {
		return const_cast<std::remove_const_t<decltype(*this)*> >(this)->
			findChild(node, leaf);
	}
	///@}
	
	/**
	 * \brief Determines whether a node contains a point.
	 */
	bool contains(ConstNodeIterator node, Vector const& point) const;
	
	/**
	 * \brief Determines whether a node contains a leaf.
	 */
	bool contains(ConstNodeIterator node, ConstLeafIterator leaf) const;
	
	/**
	 * \brief Determines whether a node contains another node.
	 */
	bool contains(ConstNodeIterator parent, ConstNodeIterator node) const;
	
};

}

#endif

#include "orthtree.tpp"

