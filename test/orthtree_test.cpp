#define BOOST_TEST_MODULE OrthtreeTest

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "glade/glade.h"

namespace bdata = boost::unit_test::data;
using namespace glade;

struct LeafValue;
struct NodeValue;

static std::size_t const Dimension = 3;
using Scalar = double;
using Point = std::array<Scalar, Dimension>;
using LeafPair = std::tuple<LeafValue, Point>;
using Octree = Orthtree<Dimension, Point, LeafValue, NodeValue>;
using RangeIndicesPair = std::pair<std::size_t, std::size_t>;

struct LeafValue {
	std::size_t data;
	explicit LeafValue(int data = 0) : data(data) {
	}
	bool operator==(LeafValue const& other) const {
		return data == other.data;
	}
	bool operator!=(LeafValue const& other) const {
		return data != other.data;
	}
};

struct NodeValue {
	std::size_t data;
	explicit NodeValue(int data = 0) : data(data) {
	}
	bool operator==(NodeValue const& other) const {
		return data == other.data;
	}
	bool operator!=(NodeValue const& other) const {
		return data != other.data;
	}
};

enum class CheckOrthtreeResult {
	Success,
	RootHasParent,
	LeafExtra,
	LeafMissing,
	DepthIncorrect,
	LeafOutOfBounds,
	NodeOverCapacity,
	NodeOverDepth,
	NodeUnderCapacity,
	ChildParentMismatch,
	LeafNotInChild,
	LeafNotInParent,
	ChildCountMismatch,
};

// Takes an orthtree and a list of leaf-position pairs that should be contained
// within it. Checks the structure of the orthtree to make sure that the leafs
// are located at appropriate locations within the orthtree.
template<
	typename LeafPair,
	std::size_t Dim, typename Vector, typename LeafValue, typename NodeValue>
static CheckOrthtreeResult checkOrthtree(
		Orthtree<Dim, Vector, LeafValue, NodeValue> const& orthtree,
		std::vector<LeafPair> allLeafPairs);

std::string to_string(Point const& point);
std::string to_string(LeafValue const& leafValue);
std::string to_string(LeafPair const& pair);
std::string to_string(Octree const& octree);
std::string to_string(RangeIndicesPair const& pair);
std::string to_string(CheckOrthtreeResult check);
template<typename T>
std::string to_string(std::vector<T> vector);

// This is needed so that Boost can print out test messages.
namespace boost {
namespace test_tools {
namespace tt_detail {

template<>
struct print_log_value<Point> {
	void operator()(std::ostream& os, Point const& point) {
		os << to_string(point);
	}
};
template<>
struct print_log_value<LeafValue> {
	void operator()(std::ostream& os, LeafValue const& leafValue) {
		os << to_string(leafValue);
	}
};
template<>
struct print_log_value<LeafPair> {
	void operator()(std::ostream& os, LeafPair const& pair) {
		os << to_string(pair);
	}
};
template<>
struct print_log_value<Octree> {
	void operator()(std::ostream& os, Octree const& octree) {
		os << to_string(octree);
	}
};
template<>
struct print_log_value<RangeIndicesPair> {
	void operator()(std::ostream& os, RangeIndicesPair const& pair) {
		os << to_string(pair);
	}
};
template<>
struct print_log_value<CheckOrthtreeResult> {
	void operator()(std::ostream& os, CheckOrthtreeResult check) {
		os << to_string(check);
	}
};

template<typename T>
struct print_log_value<std::vector<T> > {
	void operator()(std::ostream& os, std::vector<T> const& vector) {
		os << to_string(vector);
	}
};

}
}
}

template<
	typename LeafPair,
	std::size_t Dim, typename Vector, typename LeafValue, typename NodeValue>
bool compareLeafPair(
		LeafPair pair,
		typename Orthtree<Dim, Vector, LeafValue, NodeValue>::Leaf leaf) {
	return
		leaf.position == std::get<Point>(pair) &&
		leaf.value == std::get<LeafValue>(pair);
}

// A collection of different octrees with various parameters.
static auto const octreeData =
	bdata::make(Octree({0.0, 0.0, 0.0}, {16.0, 16.0, 16.0}, 3,  4)) +
	bdata::make(Octree({0.0, 0.0, 0.0}, {16.0, 16.0, 16.0}, 3,  0)) +
	bdata::make(Octree({0.0, 0.0, 0.0}, {16.0, 16.0, 16.0}, 3,  1)) +
	bdata::make(Octree({0.0, 0.0, 0.0}, {16.0, 16.0, 16.0}, 3, 64)) +
	bdata::make(Octree({0.0, 0.0, 0.0}, {16.0, 16.0, 16.0}, 1, 64)) +
	bdata::make(Octree({0.0, 0.0, 0.0}, {16.0, 16.0, 16.0}, 64, 4)) +
	bdata::make(Octree({-48.0, -32.0, -8.0}, {+64.0, +128.0, +24.0}, 3, 4));

// A set of leaf pair lists that can be used to construct octrees.
static auto const leafPairsData = bdata::make(
	std::vector<std::vector<LeafPair> > {
		// Shallow octree with a single point in each octant.
		std::vector<LeafPair> {
			LeafPair {LeafValue(0), {4,  4,  4}},
			LeafPair {LeafValue(1), {12, 4,  4}},
			LeafPair {LeafValue(2), {4,  12, 4}},
			LeafPair {LeafValue(3), {12, 12, 4}},
			LeafPair {LeafValue(4), {4,  4,  12}},
			LeafPair {LeafValue(5), {12, 4,  12}},
			LeafPair {LeafValue(6), {4,  12, 12}},
			LeafPair {LeafValue(7), {12, 12, 12}},
		},
		// Deep octree with many leafs at the same point.
		std::vector<LeafPair> {
			LeafPair {LeafValue(0), {13, 13, 13}},
			LeafPair {LeafValue(1), {13, 13, 13}},
			LeafPair {LeafValue(2), {13, 13, 13}},
			LeafPair {LeafValue(3), {13, 13, 13}},
			LeafPair {LeafValue(4), {13, 13, 13}},
		},
		// Complex octree with points in many various locations.
		std::vector<LeafPair> {
			LeafPair {LeafValue(0),  {1,  2,  1}},
			LeafPair {LeafValue(1),  {6,  2,  1}},
			LeafPair {LeafValue(2),  {6,  6,  1}},
			LeafPair {LeafValue(3),  {3,  2,  1}},
			LeafPair {LeafValue(4),  {2,  6,  1}},
			LeafPair {LeafValue(5),  {14, 6,  1}},
			LeafPair {LeafValue(6),  {6,  14, 1}},
			LeafPair {LeafValue(7),  {6,  10, 1}},
			LeafPair {LeafValue(8),  {2,  10, 1}},
			LeafPair {LeafValue(9),  {2,  14, 1}},
			
			LeafPair {LeafValue(10), {10, 6,  1}},
			LeafPair {LeafValue(11), {10, 2,  1}},
			LeafPair {LeafValue(12), {9,  9,  1}},
			LeafPair {LeafValue(13), {15, 1,  1}},
			LeafPair {LeafValue(14), {13, 3,  1}},
			LeafPair {LeafValue(15), {15, 3,  1}},
			LeafPair {LeafValue(16), {13, 1,  1}},
			LeafPair {LeafValue(17), {11, 9,  1}},
			LeafPair {LeafValue(18), {9,  11, 1}},
			LeafPair {LeafValue(19), {11, 11, 1}},
			
			LeafPair {LeafValue(20), {15, 9,  1}},
			LeafPair {LeafValue(21), {15, 13, 1}},
			LeafPair {LeafValue(22), {15, 11, 1}},
			LeafPair {LeafValue(23), {15, 15, 1}},
			LeafPair {LeafValue(24), {13, 9,  1}},
			LeafPair {LeafValue(25), {13, 13, 1}},
			LeafPair {LeafValue(26), {11, 13, 1}},
			LeafPair {LeafValue(27), {9,  13, 1}},
			LeafPair {LeafValue(28), {11, 15, 1}},
			LeafPair {LeafValue(29), {9,  15, 1}},
		},
		// A second complex octree, for variety.
		std::vector<LeafPair> {
			LeafPair {LeafValue(0),  {6.63536, 15.52272, 14.83424}},
			LeafPair {LeafValue(1),  {12.74768, 4.44096, 3.57936}},
			LeafPair {LeafValue(2),  {14.09568, 2.90976, 2.92624}},
			LeafPair {LeafValue(3),  {11.5712, 3.52352, 2.5184}},
			LeafPair {LeafValue(4),  {4.98, 2.4072, 2.1664}},
			LeafPair {LeafValue(5),  {1.79168, 3.22048, 7.66272}},
			LeafPair {LeafValue(6),  {12.95824, 9.2848, 13.46176}},
			LeafPair {LeafValue(7),  {10.57856, 11.05856, 7.7368}},
			LeafPair {LeafValue(8),  {4.20112, 15.24608, 12.00432}},
			LeafPair {LeafValue(9),  {10.03152, 8.86848, 0.13104}},
			LeafPair {LeafValue(10), {13.9248, 11.47968, 10.37936}},
			LeafPair {LeafValue(11), {14.0968, 7.6016, 7.21584}},
			LeafPair {LeafValue(12), {5.54816, 5.82736, 2.25248}},
			LeafPair {LeafValue(13), {9.83776, 6.11056, 11.17328}},
			LeafPair {LeafValue(14), {10.31104, 2.46464, 0.22048}},
		},
		// A single point.
		std::vector<LeafPair> {
			LeafPair {LeafValue(0), {4, 8, 15}},
		},
	}
);

// A list of leafs, without positions.
static auto const leafData = bdata::make(
	std::vector<LeafValue> {
		LeafValue(0),
		LeafValue(1),
		LeafValue(2),
		LeafValue(5),
		LeafValue(10),
		LeafValue(15),
		LeafValue(20),
		LeafValue(25),
		LeafValue(29),
		LeafValue(64),
	}
);

// A list of positions that are in-bounds.
static auto const positionData = bdata::make(
	std::vector<Point> {
		{ 3.1, 12.8,  8.9},
		{ 1.3,  7.1,  9.5},
		{12.5,  3.9,  2.4},
		{15.2, 12.9,  5.8},
		{ 0.7,  9.2, 13.6},
		{ 9.4,  4.7,  8.6},
		{ 4.0,  4.0,  4.0},
		{ 8.0,  3.2,  4.8},
		{ 0.1,  0.1,  0.1},
		{ 8.0,  8.0,  8.0}
	}
);

// A list of positions that are out of bounds or otherwise invalid.
static auto const invalidPositionData = bdata::make([]() {
	std::vector<Point> result {
		{-1000.0, 8.0, 8.0},
		{+1000.0, 8.0, 8.0},
		{+1000.0, +1000.0, +1000.0},
		{-1000.0, -1000.0, -1000.0},
	};
	Scalar nan = std::numeric_limits<Scalar>::quiet_NaN();
	Scalar inf = std::numeric_limits<Scalar>::infinity();
	result.push_back({inf, 0.0, 0.0});
	result.push_back({nan, 0.0, 0.0});
	result.push_back({inf, inf, inf});
	result.push_back({nan, nan, nan});
	return result;
}());

// A list of ranges (indices only).
static auto const rangeIndicesData = bdata::make(
	std::vector<RangeIndicesPair> {
		{0, 0 },
		{0, 1 },
		{0, 5 },
		{0, 10},
		{5, 5 },
		{5, 6 },
		{3, 8 },
		{2, 5 },
	}
);

// A list of positions (meant to go with the above range indices data).
static auto const positionsData = bdata::make(
	std::vector<std::vector<Point> > {
		std::vector<Point> {
		},
		std::vector<Point> {
			{10.2296, 15.62928, 4.76016},
		},
		std::vector<Point> {
			{6.67152, 2.11696, 10.47776},
			{12.96448, 3.60752, 1.22576},
			{8.73552, 8.89872, 8.34208},
			{9.09392, 7.36192, 13.19408},
			{10.26992, 7.5888, 11.07216},
		},
		std::vector<Point> {
			{10.0392, 9.53072, 4.82688},
			{6.49952, 1.1968, 3.22864},
			{5.33424, 8.40224, 8.72048},
			{0.60656, 12.80736, 12.44608},
			{2.15456, 10.44576, 0.41504},
			{2.26176, 11.33728, 4.79776},
			{11.27056, 9.78368, 11.7392},
			{5.84848, 13.05264, 11.53264},
			{0.4952, 9.6592, 9.96128},
			{5.20864, 5.29024, 11.93776},
		},
		std::vector<Point> {
		},
		std::vector<Point> {
			{15.28128, 3.77632, 8.49856},
		},
		std::vector<Point> {
			{11.14592, 15.15728, 4.62864},
			{8.09984, 2.87456, 15.62128},
			{7.16032, 14.91712, 11.2584},
			{7.74464, 12.83152, 1.40736},
			{13.05712, 1.29552, 10.93344},
		},
		std::vector<Point> {
			{8.9176, 0.65424, 14.19152},
			{5.77216, 1.36576, 11.79968},
			{10.36192, 3.26464, 12.2904},
		},
	}
);

BOOST_DATA_TEST_CASE(
		OrthtreeConstructTest,
		octreeData * leafPairsData,
		emptyOctree,
		initialLeafPairs) {
	// Construct the orthtree and then check validity.
	std::vector<LeafValue> leafValues;
	std::vector<Point> positions;
	std::transform(
		initialLeafPairs.begin(), initialLeafPairs.end(),
		std::back_inserter(leafValues),
		[](LeafPair pair) { return std::get<LeafValue>(pair); });
	std::transform(
		initialLeafPairs.begin(), initialLeafPairs.end(),
		std::back_inserter(positions),
		[](LeafPair pair) { return std::get<Point>(pair); });
	Octree octree(
		emptyOctree.root()->position,
		emptyOctree.root()->dimensions,
		leafValues.begin(),
		leafValues.end(),
		positions.begin(),
		positions.end(),
		emptyOctree.nodeCapacity(),
		emptyOctree.maxDepth());
	CheckOrthtreeResult check = checkOrthtree(octree, initialLeafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Constructs an empty orthtree, and then inserts a number of points into it.
BOOST_DATA_TEST_CASE(
		OrthtreeInsertManyTest,
		octreeData * leafPairsData,
		emptyOctree,
		initialLeafPairs) {
	// Insert the points in one at a time, and check that the orthtree is valid
	// in between each insertion.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs;
	for (auto it = initialLeafPairs.begin(); it != initialLeafPairs.end(); ++it) {
		BOOST_TEST_CHECKPOINT("preparing to insert " + to_string(*it));
		leafPairs.push_back(*it);
		octree.insertTuple(*it);
		BOOST_TEST_CHECKPOINT("finished inserting " + to_string(*it));
		CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
		BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	}
}

// Removes all of the points from an orthtree.
BOOST_DATA_TEST_CASE(
		OrthtreeEraseManyTest,
		octreeData * leafPairsData,
		emptyOctree,
		initialLeafPairs) {
	// Construct an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Now loop through all of the leaf pairs and remove them in order.
	while (!leafPairs.empty()) {
		auto leafPairIt = leafPairs.begin();
		auto octreeLeafIt = std::find_if(
			octree.leafs().begin(),
			octree.leafs().end(),
			std::bind(
				compareLeafPair<
					LeafPair,
					Dimension, Point, LeafValue, NodeValue>,
				*leafPairIt,
				std::placeholders::_1));
		LeafPair leafPair = *leafPairIt;
		BOOST_TEST_CHECKPOINT("preparing to erase " + to_string(leafPair));
		leafPairs.erase(leafPairIt);
		octree.erase(octreeLeafIt);
		BOOST_TEST_CHECKPOINT("finished erasing " + to_string(leafPair));
		CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
		BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	}
}

// Adds a single point to an orthtree.
BOOST_DATA_TEST_CASE(
		OrthtreeInsertTest,
		octreeData * leafPairsData * positionData,
		emptyOctree,
		initialLeafPairs,
		insertPosition) {
	// Construct an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	LeafPair insertLeafPair {LeafValue(0), insertPosition};
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Check the validity of the orthtree, add a single point, and then check
	// the validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	BOOST_TEST_CHECKPOINT("preparing to insert " + to_string(insertLeafPair));
	octree.insertTuple(insertLeafPair);
	leafPairs.push_back(insertLeafPair);
	BOOST_TEST_CHECKPOINT("finished inserting " + to_string(insertLeafPair));
	
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Adds a single out-of-bounds point to an orthtree.
BOOST_DATA_TEST_CASE(
		OrthtreeInsertOutOfBoundsTest,
		octreeData * leafPairsData * invalidPositionData,
		emptyOctree,
		initialLeafPairs,
		insertPosition) {
	// Construct an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	LeafPair insertLeafPair {LeafValue(0), insertPosition};
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Check the validity of the orthtree, add the invalid point, and then check
	// the validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	BOOST_TEST_CHECKPOINT("preparing to insert " + to_string(insertLeafPair));
	octree.insertTuple(insertLeafPair);
	BOOST_TEST_CHECKPOINT("finished inserting " + to_string(insertLeafPair));
	
	// Should be no change in orthtree.
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Remove a single point from an octree.
BOOST_DATA_TEST_CASE(
		OrthtreeEraseTest,
		octreeData * leafPairsData * leafData,
		emptyOctree,
		initialLeafPairs,
		eraseValue) {
	// Construct an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Check the validity of the orthtree, remove a single point, and then check
	// the validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	BOOST_TEST_CHECKPOINT("preparing to erase " + to_string(eraseValue));
	// First look for the leaf in the leafPairs list. If it is there, then
	// it should also be in the orthtree.
	auto leafPairIt = std::find_if(
		leafPairs.begin(),
		leafPairs.end(),
		[eraseValue](LeafPair pair) {
			return std::get<LeafValue>(pair) == eraseValue;
		}
	);
	if (leafPairIt != leafPairs.end()) {
		auto octreeLeafIt = std::find_if(
			octree.leafs().begin(),
			octree.leafs().end(),
			std::bind(
				compareLeafPair<
					LeafPair,
					Dimension, Point, LeafValue, NodeValue>,
				*leafPairIt,
				std::placeholders::_1));
		octree.erase(octreeLeafIt);
		leafPairs.erase(leafPairIt);
	}
	BOOST_TEST_CHECKPOINT("finished erasing " + to_string(eraseValue));
	
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Moves a point from one place in the orthtree to another.
BOOST_DATA_TEST_CASE(
		OrthtreeMoveTest,
		octreeData * leafPairsData * leafData * positionData,
		emptyOctree,
		initialLeafPairs,
		moveValue,
		movePosition) {
	// Construct an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	LeafPair moveLeafPair {moveValue, movePosition};
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Check the validity of the orthtree, move a single point, and then check
	// the validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	BOOST_TEST_CHECKPOINT("preparing to move " + to_string(moveLeafPair));
	// First look for the leaf in the leafPairs list. If it is there, then
	// it should also be in the orthtree.
	auto leafPairIt = std::find_if(
		leafPairs.begin(),
		leafPairs.end(),
		[moveValue](LeafPair pair) {
			return std::get<LeafValue>(pair) == moveValue;
		}
	);
	if (leafPairIt != leafPairs.end()) {
		auto octreeLeafIt = std::find_if(
			octree.leafs().begin(),
			octree.leafs().end(),
			std::bind(
				compareLeafPair<
					LeafPair,
					Dimension, Point, LeafValue, NodeValue>,
				*leafPairIt,
				std::placeholders::_1));
		octree.move(octreeLeafIt, movePosition);
		std::get<Point>(*leafPairIt) = movePosition;
	}
	BOOST_TEST_CHECKPOINT("finished moving " + to_string(moveLeafPair));
	
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Inserts a range of leafs.
BOOST_DATA_TEST_CASE(
		OrthtreeInsertRangeTest,
		octreeData * leafPairsData * leafPairsData,
		emptyOctree,
		initialLeafPairs,
		newLeafPairs) {
	// Construct an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	BOOST_TEST_CHECKPOINT("preparing to insert " + to_string(newLeafPairs));
	// Check the validity of the orthtree, add a bunch of points, and check the
	// validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	octree.insertTuple(newLeafPairs.begin(), newLeafPairs.end());
	leafPairs.insert(leafPairs.end(), newLeafPairs.begin(), newLeafPairs.end());
	BOOST_TEST_CHECKPOINT("finished inserting " + to_string(newLeafPairs));
	
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Erases a range of leafs.
BOOST_DATA_TEST_CASE(
		OrthtreeEraseRangeTest,
		octreeData * leafPairsData * rangeIndicesData,
		emptyOctree,
		initialLeafPairs,
		leafRangeIndices) {
	// Constructs an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Check the validity of the orthtree, erase a bunch of points, and check
	// the validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	BOOST_TEST_CHECKPOINT("preparing to erase " + to_string(leafRangeIndices));
	// Get iterators to the leaf range.
	std::size_t leafRangeIndexBegin = leafRangeIndices.first;
	std::size_t leafRangeIndexEnd = leafRangeIndices.second;
	if (leafRangeIndexEnd < octree.leafs().size()) {
		auto leafRangeBegin = octree.leafs().begin() + leafRangeIndexBegin;
		auto leafRangeEnd = octree.leafs().begin() + leafRangeIndexEnd;
		
		// Erase the leafs first from the leafPairs.
		for (auto leafIt = leafRangeBegin; leafIt != leafRangeEnd; ++leafIt) {
			LeafValue eraseValue = leafIt->value;
			auto leafPairIt = std::find_if(
				leafPairs.begin(),
				leafPairs.end(),
				[eraseValue](LeafPair pair) {
					return std::get<LeafValue>(pair) == eraseValue;
				}
			);
			leafPairs.erase(leafPairIt);
		}
		
		// Then erase from the octree.
		octree.erase(leafRangeBegin, leafRangeEnd);
	}
	BOOST_TEST_CHECKPOINT("finished erasing " + to_string(leafRangeIndices));
	
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

// Moves a range of leafs.
BOOST_DATA_TEST_CASE(
		OrthtreeMoveRangeTest,
		octreeData * leafPairsData * (rangeIndicesData ^ positionsData),
		emptyOctree,
		initialLeafPairs,
		leafRangeIndices,
		movePositions) {
	// Constructs an orthtree that contains all of the initial points.
	Octree octree = emptyOctree;
	std::vector<LeafPair> leafPairs = initialLeafPairs;
	
	BOOST_TEST_CHECKPOINT("preparing to construct orthtree");
	for (auto it = leafPairs.begin(); it != leafPairs.end(); ++it) {
		octree.insertTuple(*it);
	}
	BOOST_TEST_CHECKPOINT("finished constructing orthtree");
	
	// Check the validity of the orthtree, move a bunch of points, and check the
	// validity again.
	CheckOrthtreeResult check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
	
	BOOST_TEST_CHECKPOINT("preparing to move " + to_string(leafRangeIndices));
	// Get iterators to the leaf range.
	std::size_t leafRangeIndexBegin = leafRangeIndices.first;
	std::size_t leafRangeIndexEnd = leafRangeIndices.second;
	if (leafRangeIndexEnd < octree.leafs().size()) {
		auto leafRangeBegin = octree.leafs().begin() + leafRangeIndexBegin;
		auto leafRangeEnd = octree.leafs().begin() + leafRangeIndexEnd;
		
		// Move the leafs first in the leafPairs.
		auto positionIt = movePositions.begin();
		for (auto leafIt = leafRangeBegin; leafIt != leafRangeEnd; ++leafIt) {
			LeafValue eraseValue = leafIt->value;
			auto leafPairIt = std::find_if(
				leafPairs.begin(),
				leafPairs.end(),
				[eraseValue](LeafPair pair) {
					return std::get<LeafValue>(pair) == eraseValue;
				}
			);
			std::get<Point>(*leafPairIt) = *positionIt;
			++positionIt;
		}
		
		// Then move within the octree.
		octree.move(
			leafRangeBegin, leafRangeEnd,
			movePositions.begin(), movePositions.end());
	}
	BOOST_TEST_CHECKPOINT("finished moving " + to_string(leafRangeIndices));
	
	check = checkOrthtree(octree, leafPairs);
	BOOST_REQUIRE_EQUAL(check, CheckOrthtreeResult::Success);
}

template<
	typename LeafPair,
	std::size_t Dim, typename Vector, typename LeafValue, typename NodeValue>
CheckOrthtreeResult checkOrthtree(
		Orthtree<Dim, Vector, LeafValue, NodeValue> const& orthtree,
		std::vector<LeafPair> allLeafPairs) {
	// Create a stack storing the points that belong to the current node.
	std::vector<std::vector<LeafPair> > leafPairsStack;
	leafPairsStack.push_back(allLeafPairs);
	
	// Check that the root node has no parent.
	if (orthtree.root()->hasParent) {
		return CheckOrthtreeResult::RootHasParent;
	}
	
	// Loop through all of the nodes.
	for (
			auto node = orthtree.root();
			node != orthtree.nodes().end();
			++node) {
		// Check that the current node has depth of +1 from its parent.
		if (node->depth != (node->hasParent ? node->parent->depth + 1 : 0)) {
			return CheckOrthtreeResult::DepthIncorrect;
		}
		
		// Take the top of the stack, and check whether each of the
		// leaf-position pairs are within the dimensions.
		std::vector<LeafPair> leafPairs(leafPairsStack.back());
		leafPairsStack.pop_back();
		
		if (leafPairs.size() > node->leafs.size()) {
			return CheckOrthtreeResult::LeafMissing;
		}
		if (leafPairs.size() < node->leafs.size()) {
			return CheckOrthtreeResult::LeafExtra;
		}
		
		for (auto leafPair : leafPairs) {
			// First, find the leaf within the node.
			auto leaf = std::find_if(
				node->leafs.begin(),
				node->leafs.end(),
				std::bind(
					compareLeafPair<
						LeafPair,
						Dim, Vector, LeafValue, NodeValue>,
					leafPair,
					std::placeholders::_1));
			if (leaf == node->leafs.end()) {
				return CheckOrthtreeResult::LeafMissing;
			}
			// Then, make sure that it is contained within the bounds of the
			// node.
			for (std::size_t dim = 0; dim < Dim; ++dim) {
				Scalar position = node->position[dim];
				Scalar dimensions = node->dimensions[dim];
				if (!(
						leaf->position[dim] >= position &&
						leaf->position[dim] - position < dimensions)) {
					return CheckOrthtreeResult::LeafOutOfBounds;
				}
			}
		}
		
		// Next, check that the node's children contain all of its leafs. All of
		// of the node's leafs should belong to one and only one child.
		bool overCapacity =
			node->leafs.size() > orthtree.nodeCapacity();
		if (!node->hasChildren) {
			int depthSign =
				static_cast<int>(node->depth > orthtree.maxDepth()) -
				static_cast<int>(node->depth < orthtree.maxDepth());
			// If the node doesn't have children, then make sure that it doesn't
			// have too many leafs and that it isn't too deep.
			if (depthSign < 0 && overCapacity) {
				return CheckOrthtreeResult::NodeOverCapacity;
			}
			if (depthSign > 0) {
				return CheckOrthtreeResult::NodeOverDepth;
			}
		}
		else {
			// Otherwise, make sure it doens't have too few leafs either.
			if (!overCapacity) {
				return CheckOrthtreeResult::NodeUnderCapacity;
			}
			// Iterate over every child, and add its leafs to the stack (in
			// reverse order so that the children are added to the stack in
			// order).
			for (std::size_t childIndex = (1 << Dim); childIndex-- > 0; ) {
				auto child = node->children[childIndex];
				
				// Check that the child's parent is this node.
				if (child->parent != node) {
					return CheckOrthtreeResult::ChildParentMismatch;
				}
				// Create a vector to store the leaf-position pairs that belong
				// to the child.
				std::vector<LeafPair> childLeafPairs;
				
				// Remove the child's leafs from leafPairs and put them into
				// childLeafPairs instead.
				auto lastChildLeaf = std::partition(
					leafPairs.begin(),
					leafPairs.end(),
					[&orthtree, &child](LeafPair leafPair) {
						return child->leafs.end() != std::find_if(
							child->leafs.begin(),
							child->leafs.end(),
							std::bind(
								compareLeafPair<
									LeafPair,
									Dim, Vector, LeafValue, NodeValue>,
								leafPair,
								std::placeholders::_1));
					});
				std::copy(
					leafPairs.begin(),
					lastChildLeaf,
					std::back_inserter(childLeafPairs));
				leafPairs.erase(leafPairs.begin(), lastChildLeaf);
				
				// Put the child leaf pairs onto the stack.
				if (childLeafPairs.size() != child->leafs.size()) {
					return CheckOrthtreeResult::LeafNotInParent;
				}
				leafPairsStack.push_back(childLeafPairs);
			}
			// Check that each of the leaf-position pairs belonged to at least
			// one of the children.
			if (!leafPairs.empty()) {
				return CheckOrthtreeResult::LeafNotInChild;
			}
		}
	}
	
	// The stack should be empty, except if one of the nodes didn't have the
	// right number of children.
	if (!leafPairsStack.empty()) {
		return CheckOrthtreeResult::ChildCountMismatch;
	}
	
	return CheckOrthtreeResult::Success;
}

std::string to_string(Point const& point) {
	std::ostringstream os;
	os << "<" << point[0];
	for (std::size_t dim = 1; dim < point.size(); ++dim) {
		os << ", " << point[dim];
	}
	os << ">";
	return os.str();
}

std::string to_string(LeafValue const& leaf) {
	std::ostringstream os;
	os << "LeafValue(" << leaf.data << ")";
	return os.str();
}

std::string to_string(LeafPair const& pair) {
	std::ostringstream os;
	os << "(";
	os << to_string(std::get<LeafValue>(pair)) << ", ";
	os << to_string(std::get<Point>(pair)) << ")";
	return os.str();
}

std::string to_string(Octree const& octree) {
	std::ostringstream os;
	os << "Octree(";
	os << "node capacity: " << octree.nodeCapacity() << ", ";
	os << "max depth: " << octree.maxDepth() << ", ";
	os << "position: ";
	os << to_string(octree.root()->position);
	os << ", ";
	os << "dimensions: ";
	os << to_string(octree.root()->dimensions);
	os << ")";
	return os.str();
}

std::string to_string(RangeIndicesPair const& pair) {
	std::ostringstream os;
	os << "Range(";
	os << pair.first << ", ";
	os << pair.second << ")";
	return os.str();
}

template<typename T>
std::string to_string(std::vector<T> vector) {
	std::ostringstream os;
	os << "{";
	if (!vector.empty()) {
		os << to_string(vector[0]);
	}
	for (std::size_t index = 1; index < vector.size(); ++index) {
		os << ", ";
		os << to_string(vector[index]);
	}
	os << "}";
	return os.str();
}

std::string to_string(CheckOrthtreeResult check) {
	std::string error;
	switch (check) {
	case CheckOrthtreeResult::Success:
		error = "success";
		break;
	case CheckOrthtreeResult::RootHasParent:
		error = "root node has parent";
		break;
	case CheckOrthtreeResult::LeafExtra:
		error = "node contains extra leafs";
		break;
	case CheckOrthtreeResult::LeafMissing:
		error = "node is missing leaf";
		break;
	case CheckOrthtreeResult::DepthIncorrect:
		error = "node has incorrect depth";
		break;
	case CheckOrthtreeResult::LeafOutOfBounds:
		error = "leaf position not inside node boundary";
		break;
	case CheckOrthtreeResult::NodeOverCapacity:
		error = "node over max capacity";
		break;
	case CheckOrthtreeResult::NodeOverDepth:
		error = "node over max depth";
		break;
	case CheckOrthtreeResult::NodeUnderCapacity:
		error = "node's children are unnecessary";
		break;
	case CheckOrthtreeResult::ChildParentMismatch:
		error = "child's parent reference is incorrect";
		break;
	case CheckOrthtreeResult::LeafNotInChild:
		error = "node's leaf is not in children";
		break;
	case CheckOrthtreeResult::LeafNotInParent:
		error = "child node's leaf is not in parent";
		break;
	case CheckOrthtreeResult::ChildCountMismatch:
		error = "node had incorrect child count";
		break;
	default:
		error = "";
		break;
	}
	return error;
}

