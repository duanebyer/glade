#ifndef __GLADE_ORTHTREE_TPP_
#define __GLADE_ORTHTREE_TPP_

#include "orthtree.h"

#include "orthtree_iterator.h"
#include "orthtree_range.h"
#include "orthtree_reference.h"
#include "orthtree_value.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <stack>
#include <tuple>
#include <utility>
#include <vector>

namespace glade {

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::Orthtree(
		Vector position,
		Vector dimensions,
		LeafListSizeType nodeCapacity,
		NodeListSizeType maxDepth,
		bool autoAdjust) :
		_leafs(),
		_nodes(),
		_nodeCapacity(nodeCapacity),
		_maxDepth(maxDepth),
		_autoAdjust(autoAdjust) {
	NodeInternal root(position, dimensions);
	_nodes.insert(_nodes.begin(), root);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<typename LeafIt, typename PositionIt>
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::Orthtree(
		Vector position,
		Vector dimensions,
		LeafIt leafBegin,
		LeafIt leafEnd,
		PositionIt positionBegin,
		PositionIt positionEnd,
		LeafListSizeType nodeCapacity,
		NodeListSizeType maxDepth,
		bool autoAdjust) :
		Orthtree(position, dimensions, nodeCapacity, maxDepth, autoAdjust) {
	(void) positionEnd;
	typename std::iterator_traits<LeafIt>::difference_type numLeafs =
		std::distance(leafBegin, leafEnd);
	reserve(numLeafs);
	
	// Add leafs to the leaf vector.
	PositionIt positionIt = positionBegin;
	for (LeafIt leafIt = leafBegin; leafIt != leafEnd; ++leafIt, ++positionIt) {
		_leafs.push_back(LeafInternal(*positionIt, *leafIt));
	}
	_nodes[0].leafIndex = 0;
	_nodes[0].leafCount = _leafs.size();
	
	// Get access to the current node.
	NodeIterator node = root();
	while (node != nodes().end()) {
		if (!canHoldLeafs(node, 0)) {
			// Create a new set of child nodes.
			allocChildren(node);
			updateNodeChildData(node, true);
			// Assign each of the leafs from the current node into a child.
			std::vector<LeafInternal> newLeafsByChild[1 << Dim];
			for (std::size_t dim = 0; dim < (1 << Dim); ++dim) {
				newLeafsByChild[dim].reserve(node->leafs.size());
			}
			for (
					LeafIterator leaf = node->leafs.begin();
					leaf != node->leafs.end();
					++leaf) {
				NodeIterator child = findChild(node, leaf->position);
				newLeafsByChild[child.internalIt()->siblingIndex].push_back(
					*leaf.internalIt());
			}
			// Copy the new child leaf vectors into master leaf vector. Also
			// update nodes.
			LeafListSizeType leafIndex = node->leafs.begin()._index;
			for (std::size_t dim = 0; dim < (1 << Dim); ++dim) {
				std::vector<LeafInternal>& newLeafs = newLeafsByChild[dim];
				node->children[dim].internalIt()->leafIndex = leafIndex;
				node->children[dim].internalIt()->leafCount = newLeafs.size();
				std::copy(
					newLeafs.begin(),
					newLeafs.end(),
					_leafs.begin() + leafIndex);
				leafIndex += newLeafs.size();
			}
		}
		++node;
	}
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::createChildren(
		NodeIterator node) {
	allocChildren(node);
	updateNodeChildData(node, true);
	distributeLeafs(node);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::destroyChildren(
		NodeIterator node) {
	freeChildren(node);
	updateNodeChildData(node, false);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::allocChildren(
		NodeIterator node) {
	// Create the 2^Dim child nodes inside the list of nodes. This will not
	// invalidate the node iterator.
	_nodes.insert(
		node.internalIt() + 1,
		1 << Dim,
		NodeInternal(node->position, node->dimensions));
	
	// Loop through the new children, and set up their various properties.
	for (std::size_t index = 0; index < (1 << Dim); ++index) {
		NodeInternal& child = *(node.internalIt() + index + 1);
		child.depth = node->depth + 1;
		child.parentIndex = -(static_cast<NodeListDifferenceType>(index) + 1);
		child.siblingIndex = index;
		child.leafIndex =
			node.internalIt()->leafIndex +
			node.internalIt()->leafCount;
		// Position and size the child node.
		for (std::size_t dim = 0; dim < Dim; ++dim) {
			child.dimensions[dim] = child.dimensions[dim] / 2;
			if ((1 << dim) & index) {
				child.position[dim] =
					child.position[dim] + node->dimensions[dim] / 2;
			}
		}
	}
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::freeChildren(
		NodeIterator node) {
	_nodes.erase(
		node->children[0].internalIt(),
		node->children[1 << Dim].internalIt());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
NodeListDifferenceType
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::updateNodeChildData(
		NodeIterator node,
		bool childrenCreated,
		bool updateParentIndices) {
	// Update the original node's references to its children first.
	node.internalIt()->hasChildren = childrenCreated;
	auto childIndicesBegin = node.internalIt()->childIndices;
	auto childIndicesEnd = node.internalIt()->childIndices + (1 << Dim) + 1;
	NodeListDifferenceType childCountChange = -*(childIndicesEnd - 1);
	if (childrenCreated) {
		std::iota(childIndicesBegin, childIndicesEnd, 1);
	}
	else {
		std::fill(childIndicesBegin, childIndicesEnd, 1);
	}
	childCountChange += *(childIndicesEnd - 1);
	
	return updateAncestorChildData(node, childCountChange, updateParentIndices);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::
NodeListDifferenceType
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::updateAncestorChildData(
		NodeIterator node,
		NodeListDifferenceType childCountChange,
		bool updateParentIndices) {
	// Go through the parent, grandparent, great-grandparent, ... of this node
	// and update their child indices.
	NodeIterator parent = node;
	while (parent->hasParent) {
		NodeListSizeType siblingIndex = parent.internalIt()->siblingIndex;
		parent = parent->parent;
		while (++siblingIndex < (1 << Dim)) {
			parent.internalIt()->childIndices[siblingIndex] += childCountChange;
			NodeIterator child = parent->children[siblingIndex];
			// Only update parent indices if requested.
			if (updateParentIndices) {
				child.internalIt()->parentIndex -= childCountChange;
			}
		}
		parent.internalIt()->childIndices[1 << Dim] += childCountChange;
	}
	return childCountChange;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::distributeLeafs(
		NodeIterator node) {
	// Distribute the leaves of this node to the children.
	for (LeafListSizeType index = 0; index < node->leafs.size(); ++index) {
		NodeListSizeType childIndex = 0;
		// The leaf is always taken from the front of the list, and moved to
		// a location on the end of the list to prevent double processing.
		LeafIterator leaf = node->leafs.begin();
		NodeIterator child = findChild(node, leaf->position);
		moveAt(node, child, leaf);
	}
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::insertAt(
		NodeIterator node,
		LeafValue const& value,
		Vector const& position) {
	// Add the leaf to the master list of leaves in the orthtree.
	_leafs.insert(
		node->leafs.end().internalIt(),
		LeafInternal(position, value));
	
	// Loop through the rest of the nodes and increment their leaf indices
	// so that they still refer to the correct location in the leaf vector.
	auto currentNode = node.internalIt();
	while (++currentNode != _nodes.end()) {
		++currentNode->leafIndex;
	}
	
	// Also loop through all ancestors and increment their leaf counts.
	NodeIterator parent = node;
	bool hasParent = true;
	while (hasParent) {
		++parent.internalIt()->leafCount;
		hasParent = parent->hasParent;
		parent = parent->parent;
	}
	
	LeafIterator result = node->leafs.end(); --result;
	return result;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::eraseAt(
		NodeIterator node,
		LeafIterator leaf) {
	// Remove the leaf from the master orthtree leaf vector.
	_leafs.erase(leaf.internalIt());
	
	// Loop through the rest of the nodes and increment their leaf indices
	// so that they still refer to the correct location in the leaf vector.
	auto currentNode = node.internalIt();
	while (++currentNode != _nodes.end()) {
		--currentNode->leafIndex;
	}
	
	// Loop through all of the ancestors of this node and decremement their
	// leaf counts.
	NodeIterator parent = node;
	bool hasParent = true;
	while (hasParent) {
		--parent.internalIt()->leafCount;
		hasParent = parent->hasParent;
		parent = parent->parent;
	}
	
	return leaf;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::moveAt(
		NodeIterator sourceNode,
		NodeIterator destNode,
		LeafIterator sourceLeaf) {
	LeafIterator destLeaf = destNode->leafs.end();
	LeafIterator result = sourceLeaf;
	// Determine the relative order of the source and destination iterators.
	if (destLeaf > sourceLeaf) {
		// Perform a left rotation.
		LeafInternal oldLeafInternal = *sourceLeaf.internalIt();
		std::move(
			sourceLeaf.internalIt() + 1,
			destLeaf.internalIt(),
			sourceLeaf.internalIt());
		result = destLeaf - 1;
		*result.internalIt() = oldLeafInternal;
	}
	else if (destLeaf < sourceLeaf) {
		// Perform a right rotation.
		LeafInternal oldLeafInternal = *sourceLeaf.internalIt();
		std::move_backward(
			destLeaf.internalIt(),
			sourceLeaf.internalIt(),
			sourceLeaf.internalIt() + 1);
		result = destLeaf;
		*result.internalIt() = oldLeafInternal;
	}
	
	// Adjust the ancestors of the destination node to all have one more leaf.
	NodeIterator destParentNode = destNode;
	++destNode.internalIt()->leafCount;
	while (destParentNode->hasParent) {
		destParentNode = destParentNode->parent;
		++destParentNode.internalIt()->leafCount;
	}
	
	// Adjust the ancestors of the source node to all have one less leaf. Note
	// that integer underflow will not occur because of the previous loop.
	NodeIterator sourceParentNode = sourceNode;
	--sourceNode.internalIt()->leafCount;
	while (sourceParentNode->hasParent) {
		sourceParentNode = sourceParentNode->parent;
		--sourceParentNode.internalIt()->leafCount;
	}
	
	// Adjust the leaf indices of the nodes between the source and destination.
	bool nodeInverted = (sourceNode >= destNode);
	signed char leafIndexOffset = nodeInverted ? +1 : -1;
	NodeIterator firstNode = nodeInverted ? destNode : sourceNode;
	NodeIterator lastNode = nodeInverted ? sourceNode : destNode;
	++firstNode;
	++lastNode;
	for (NodeIterator node = firstNode; node != lastNode; ++node) {
		node.internalIt()->leafIndex += leafIndexOffset;
	}
	
	return result;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::leafs() {
	return LeafRange(this, 0, _leafs.size());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstLeafRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::leafs() const {
	return ConstLeafRange(this, 0, _leafs.size());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstLeafRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::cleafs() const {
	return ConstLeafRange(this, 0, _leafs.size());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::nodes() {
	return NodeRange(this, 0, _nodes.size());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstNodeRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::nodes() const {
	return ConstNodeRange(this, 0, _nodes.size());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstNodeRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::cnodes() const {
	return ConstNodeRange(this, 0, _nodes.size());
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::root() {
	return NodeIterator(this, 0);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstNodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::root() const {
	return ConstNodeIterator(this, 0);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstNodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::croot() const {
	return ConstNodeIterator(this, 0);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::descendants(
		ConstNodeIterator node) {
	return NodeRange(
		this,
		node->children[0]._index,
		node->children[1 << Dim]._index);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstNodeRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::descendants(
		ConstNodeIterator node) const {
	return ConstNodeRange(
		this,
		node->children[0]._index,
		node->children[1 << Dim]._index);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::ConstNodeRange
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::cdescendants(
		ConstNodeIterator node) const {
	return ConstNodeRange(
		this,
		node->children[0]._index,
		node->children[1 << Dim]._index);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
bool Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::adjust(
		NodeIterator node) {
	// To do this in linear time, the current nodes need to be copied (with
	// adjustments) into a new vector.
	bool result = false;
	
	// Create a new orthtree with 'node' as the root to store the modified part.
	Orthtree<Dim, Vector, LeafValue, NodeValue, Details> newOrthtree(
		Vector(),
		Vector(),
		_nodeCapacity,
		_maxDepth,
		false);
	// Reserve space in the new orthtree to hold copies of the node/leaf data.
	newOrthtree.reserve(node->leafs.size());
	newOrthtree._nodes.reserve(2 * (descendants(node).size() + 1));
	std::copy(
		node->leafs.begin().internalIt(),
		node->leafs.end().internalIt(),
		std::back_inserter(newOrthtree._leafs));
	
	// Set the root to be the node we are adjusting.
	*newOrthtree.root().internalIt() = *node.internalIt();
	LeafListSizeType leafOffset = node.internalIt()->leafIndex;
	newOrthtree.root().internalIt()->leafIndex -= leafOffset;
	
	// Create a new set of adjusted nodes from the old set of nodes.
	NodeIterator oldNode = node;
	NodeIterator newNode = newOrthtree.root();
	std::stack<NodeListDifferenceType> parentOffsets;
	parentOffsets.push(0);
	NodeListSizeType depth = newNode->depth;
	while (newNode != newOrthtree.nodes().end()) {
		// Adjust the parent offsets stack depending on whether we're going up
		// or down in the orthtree.
		while (newNode->depth > depth) {
			++depth;
			parentOffsets.push(0);
		}
		while (newNode->depth < depth) {
			--depth;
			NodeListDifferenceType lastTop = parentOffsets.top();
			parentOffsets.pop();
			parentOffsets.top() += lastTop;
		}
		
		// Adjust the parent offset.
		newNode.internalIt()->parentIndex += parentOffsets.top();
		
		// If the node doesn't have children but should, create them.
		if (!newNode->hasChildren && !newOrthtree.canHoldLeafs(newNode, 0)) {
			result = true;
			newOrthtree.allocChildren(newNode);
			parentOffsets.top() -= newOrthtree.updateNodeChildData(
				newNode, true, false);
			newOrthtree.distributeLeafs(newNode);
		}
		// If the node does have children but shouldn't, destroy them.
		else if (newNode->hasChildren && newOrthtree.canHoldLeafs(newNode, 0)) {
			result = true;
			parentOffsets.top() -= newOrthtree.updateNodeChildData(
				newNode, false, false);
			// Skip the remaining children from the old list of nodes, since we
			// won't be adding them anyway.
			oldNode = oldNode->children[(1 << Dim)];
			--oldNode;
		}
		
		// Continue to the next new node.
		++newNode;
		// If we've run out of nodes to process in the new set of nodes, then
		// take the next node from the old set.
		if (
				newNode == newOrthtree.nodes().end() &&
				oldNode + 1 != node->children[1 << Dim]) {
			++oldNode;
			newOrthtree._nodes.push_back(*oldNode.internalIt());
			newNode.internalIt()->leafIndex -= leafOffset;
		}
	}
	
	// Resize the section of the nodes vector used to store 'node' and its
	// descendants so it can hold the new descendants.
	NodeListDifferenceType change = newOrthtree.nodes().size();
	change -= (1 + descendants(node).size());
	if (change < 0) {
		_nodes.erase(
			node.internalIt(),
			node.internalIt() + std::abs(change));
	}
	else if (change > 0) {
		_nodes.insert(
			node.internalIt(),
			std::abs(change),
			NodeInternal(Vector(), Vector()));
	}
	// Copy the new descendants into the node vector. Also overwrite the section
	// of the leafs vector to account for changes due to creating children.
	if (leafOffset != 0) {
		std::for_each(
			newOrthtree._nodes.begin(),
			newOrthtree._nodes.end(),
			[leafOffset](NodeInternal& nodeInternal) {
				nodeInternal.leafIndex += leafOffset;
			});
	}
	std::copy(
		newOrthtree._nodes.begin(),
		newOrthtree._nodes.end(),
		node.internalIt());
	std::copy(
		newOrthtree._leafs.begin(),
		newOrthtree._leafs.end(),
		node->leafs.begin().internalIt());
	
	// Adjust all the ancestors of the node so they see the change in number of
	// children.
	updateAncestorChildData(node, change + 1);
	
	return result;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
std::tuple<
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator,
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafIterator>
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::insert(
		ConstNodeIterator hint,
		LeafValue value,
		Vector const& position) {
	// Find the node with the correct position, and insert the leaf into
	// that node.
	NodeIterator node = find(hint, position);
	if (node == nodes().end()) {
		return std::make_tuple(nodes().end(), leafs().end());
	}
	// Create children if the node doesn't have the capacity to store
	// this leaf.
	while (_autoAdjust && !canHoldLeafs(node, +1)) {
		createChildren(node);
		node = find(node, position);
	}
	return std::make_tuple(node, insertAt(node, value, position));
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<typename LeafIt, typename PositionIt>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::insert(
		ConstNodeIterator hint,
		LeafIt leafBegin, LeafIt leafEnd,
		PositionIt positionBegin, PositionIt positionEnd) {
	// Add every leaf to the orthtree (without adjustment).
	(void) positionEnd;
	reserve(std::distance(leafBegin, leafEnd));
	autoAdjust(false);
	PositionIt positionIt = positionBegin;
	for (LeafIt leafIt = leafBegin; leafIt != leafEnd; ++leafIt, ++positionIt) {
		insert(hint, *leafIt, *positionIt);
	}
	// Then adjust.
	adjust();
	autoAdjust(true);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
std::tuple<
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator,
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafIterator>
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::erase(
		ConstNodeIterator hint,
		LeafIterator leaf) {
	// Find the node that contains this leaf, and then erase the leaf from
	// that node.
	NodeIterator node = find(hint, leaf);
	if (node == nodes().end()) {
		return std::make_tuple(nodes().end(), leafs().end());
	}
	// If the parent of this node doesn't need to be divided into subnodes
	// anymore, then merge its children together.
	while (
			_autoAdjust &&
			node->hasParent &&
			canHoldLeafs(node->parent, -1)) {
		node = node->parent;
		destroyChildren(node);
	}
	return std::make_tuple(node, eraseAt(node, leaf));
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::erase(
		ConstNodeIterator hint,
		LeafIterator leafBegin, LeafIterator leafEnd) {
	// Erase the leafs in reverse, to prevent invalidation of the iterators
	// (without adjustment).
	autoAdjust(false);
	for (
			ReverseLeafIterator leafIt = leafEnd.reverse();
			leafIt != leafBegin.reverse();
			++leafIt) {
		erase(hint, leafIt.reverse() - 1);
	}
	// Then adjust.
	adjust();
	autoAdjust(true);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
std::tuple<
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator,
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator,
	typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::LeafIterator>
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::move(
		ConstNodeIterator hint,
		LeafIterator leaf,
		Vector const& position) {
	// Find the source node that contains the leaf, and the target node with
	// the correct position.
	NodeIterator source = find(hint, leaf);
	NodeIterator dest = find(hint, position);
	if (source == nodes().end() || dest == nodes().end()) {
		return std::make_tuple(nodes().end(), nodes().end(), leafs().end());
	}
	// If the source and the destination are distinct, then check to make
	// sure that they remain within the node capacity. If they won't, then
	// create or destroy children until they do.
	if (_autoAdjust) {
		while (
				source->hasParent &&
				canHoldLeafs(source->parent, -1) &&
				!contains(source->parent, dest)) {
			// If dest will become invalidated by destroying children, then
			// adjust it so it will still be valid.
			if (dest > source) {
				dest -= (1 << Dim);
			}
			source = source->parent;
			destroyChildren(source);
		}
		while (!canHoldLeafs(dest, +1) && dest != source) {
			// If source will become invalidated by creating children, then
			// adjust it so it will still be valid.
			if (source > dest) {
				source += (1 << Dim);
			}
			createChildren(dest);
			dest = findChild(dest, position);
		}
	}
	// Move the leaf.
	leaf.internalIt()->position = position;
	return std::make_tuple(source, dest, moveAt(source, dest, leaf));
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
template<typename PositionIt>
void Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::move(
		ConstNodeIterator hint,
		LeafIterator leafBegin, LeafIterator leafEnd,
		PositionIt positionBegin, PositionIt positionEnd) {
	(void) positionEnd;
	// Move the leafs one by one (without adjustment).
	autoAdjust(false);
	LeafListDifferenceType numLeafs = std::distance(leafBegin, leafEnd);
	std::vector<bool> processed(numLeafs, false);
	LeafListDifferenceType processedIndex = 0;
	LeafIterator leafIt = leafBegin;
	PositionIt positionIt = positionBegin;
	for (
			LeafListDifferenceType leafIndex = 0;
			leafIndex < numLeafs;
			++leafIndex) {
		// Move the leaf and get an iterator to its new position.
		LeafIterator newLeafIt =
			std::get<LeafIterator>(move(hint, leafIt, *positionIt));
		LeafListDifferenceType newProcessedIndex = (newLeafIt - leafBegin);
		// If the leaf is moved into the range that still has to be processed,
		// then tag it so that it isn't processed twice.
		if (newProcessedIndex <= processedIndex) {
			processed[processedIndex] = true;
		}
		else if (newProcessedIndex < numLeafs) {
			// Perform a left rotation on the processed flags.
			std::move(
				processed.begin() + processedIndex + 1,
				processed.begin() + newProcessedIndex + 1,
				processed.begin() + processedIndex);
			processed[newProcessedIndex] = true;
		}
		else {
			// Perform a left rotation but don't flag the new processed since
			// it is so far to the right it won't be encountered again.
			std::move(
				processed.begin() + processedIndex + 1,
				processed.end(),
				processed.begin() + processedIndex);
		}
		while (processed[processedIndex]) {
			++processedIndex;
			++leafIt;
		}
		++positionIt;
	}
	// Then adjust.
	adjust();
	autoAdjust(true);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::find(
		ConstNodeIterator hint,
		Vector const& point) {
	// If the hint node doesn't contain the point, then go up the tree until we
	// reach a node that does contain the point.
	NodeIterator node(this, hint._index);
	while (!contains(node, point)) {
		if (node->hasParent) {
			node = node->parent;
		}
		else {
			return nodes().end();
		}
	}
	
	// Then, go down the tree until we reach the deepest node that contains the
	// point.
	while (node->hasChildren) {
		node = findChild(node, point);
	}
	
	return node;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::find(
		ConstNodeIterator hint,
		ConstLeafIterator leaf) {
	// If the hint node doesn't contain the leaf, then go up the tree until we
	// reach a node that does contain the leaf.
	NodeIterator node(this, hint._index);
	while (!contains(node, leaf)) {
		if (node->hasParent) {
			node = node->parent;
		}
		else {
			return nodes().end();
		}
	}
	
	// Then go down the tree until we reach the deepest node that contains the
	// point.
	while (node->hasChildren) {
		node = findChild(node, leaf);
	}
	
	return node;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::findChild(
		ConstNodeIterator node,
		Vector const& point) {
	NodeListSizeType childIndex = 0;
	for (std::size_t dim = 0; dim < Dim; ++dim) {
		if (point[dim] - node->position[dim] >= node->dimensions[dim] / 2) {
			childIndex += (1 << dim);
		}
	}
	ConstNodeIterator child = node->children[childIndex];
	return NodeIterator(this, child._index);
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
typename Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::NodeIterator
Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::findChild(
		ConstNodeIterator node,
		ConstLeafIterator leaf) {
	for (
			NodeListSizeType childIndex = 0;
			childIndex < (1 << Dim);
			++childIndex) {
		ConstNodeIterator child = node->children[childIndex];
		if (contains(child, leaf)) {
			return NodeIterator(this, child._index);
		}
	}
	return nodes().end();
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
bool Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::contains(
		ConstNodeIterator node,
		Vector const& point) const {
	for (std::size_t dim = 0; dim < Dim; ++dim) {
		Scalar lower = node->position[dim];
		Scalar upper = lower + node->dimensions[dim];
		if (!(
				point[dim] >= node->position[dim] &&
				point[dim] - node->position[dim] < node->dimensions[dim])) {
			return false;
		}
	}
	return true;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
bool Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::contains(
		ConstNodeIterator node,
		ConstLeafIterator leaf) const {
	LeafListSizeType leafIndex = leaf._index;
	LeafListSizeType lower = node.internalIt()->leafIndex;
	LeafListSizeType upper = lower + node.internalIt()->leafCount;
	return leafIndex >= lower && leafIndex < upper;
}

template<
	std::size_t Dim,
	typename Vector,
	typename LeafValue,
	typename NodeValue,
	typename Details>
bool Orthtree<Dim, Vector, LeafValue, NodeValue, Details>::contains(
		ConstNodeIterator parent,
		ConstNodeIterator node) const {
	ConstNodeIterator nodeParent = node;
	NodeListSizeType parentDepth = parent.internalIt()->depth;
	while (nodeParent.internalIt()->depth > parentDepth) {
		nodeParent = nodeParent->parent;
	}
	return nodeParent == parent;
}

}

#endif

