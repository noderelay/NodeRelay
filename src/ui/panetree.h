#pragma once
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <Qt>
#include <vector>

// A pane layout as a tree instead of a set of hand-written shapes.
//
// A leaf holds a slot index into the caller's list of views; a split holds an
// axis and its children. Slots rather than widgets keep this header free of
// Qt Widgets, so the shapes can be tested without a GUI.
struct PaneNode {
    int             slot{-1};                  // leaf only: index into the view list
    Qt::Orientation axis{Qt::Horizontal};      // split only: how children are laid out
    std::vector<PaneNode> children;            // empty on a leaf
    // Split only: each child's share of this split, parallel to children and
    // summing to 1. Empty means "even", which is what every new split starts
    // as; dragging a splitter writes real numbers back.
    std::vector<double> fractions;

    bool isLeaf() const { return children.empty(); }
};

// Even shares for a split's children, used whenever fractions are missing or
// don't line up with the child count.
std::vector<double> evenFractions(size_t count);

// The layout for `count` views, reproducing the shapes Uplink has always used:
// 1 or 2 flat along the main axis, 3 as one full-length view beside a stacked
// pair, 4 as two stacked pairs. `rows` transposes all of them 90°.
//
// The root is always a split so the caller can hand its children straight to
// the existing top-level splitter. A count of 0 gives a childless root.
PaneNode buildShapeTree(int count, bool rows);

// ---------------------------------------------------------------------------
// Tree edits. Ids are the caller's own identifiers for its views; the tree
// neither creates nor owns them. Every edit renormalises: a split left with one
// child collapses into it, and a split nested directly inside a split of the
// same axis is flattened into its parent, so three views dropped one below the
// other end up as three equal rows rather than a lopsided pile.
// ---------------------------------------------------------------------------

// Leaf ids in layout order.
std::vector<int> leafOrder(const PaneNode &root);
bool containsLeaf(const PaneNode &root, int id);

// Splits the leaf holding `targetId` in two along `axis`, putting `newId` on
// the near side when `before` is set. Returns false if the target isn't there.
bool splitLeaf(PaneNode &root, int targetId, int newId, Qt::Orientation axis, bool before);

// Exchanges the positions of two leaves. Returns false unless both exist.
bool swapLeaves(PaneNode &root, int a, int b);

// Removes a leaf, collapsing whatever that empties.
bool removeLeaf(PaneNode &root, int id);

// Moves an existing leaf to sit beside another: a remove followed by a split,
// which is what an edge-drop does. Refuses to move a leaf onto itself.
bool moveLeafBeside(PaneNode &root, int movingId, int targetId,
                    Qt::Orientation axis, bool before);

// Rebuilds the tree as a single split along `axis` holding every leaf in its
// current order — what "always columns" and "always rows" mean once layouts
// can nest.
PaneNode flattenTree(const PaneNode &root, Qt::Orientation axis);

bool operator==(const PaneNode &a, const PaneNode &b);
inline bool operator!=(const PaneNode &a, const PaneNode &b) { return !(a == b); }

// ---------------------------------------------------------------------------
// Layout persistence. Leaves are stored under caller-provided string keys —
// slot ids are handed out per run and mean nothing across a restart.
// ---------------------------------------------------------------------------

QJsonObject paneTreeToJson(const PaneNode &root, const QHash<int, QString> &keyForSlot);

// Rebuilds a tree, resolving each stored key back to a live slot. Leaves
// whose key didn't come back are dropped, a key appearing twice keeps only
// its first leaf (a hand-edited layout must not yield a duplicate-slot
// tree), and a split left with nothing is dropped with its leaves. Splits
// of one collapse and bad size lists fall back to even fractions, same
// rules as the tree edits. Returns false when nothing survives.
bool paneTreeFromJson(const QJsonObject &obj, const QHash<QString, int> &idForKey,
                      PaneNode &out);
