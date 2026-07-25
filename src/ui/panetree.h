#pragma once
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

    bool isLeaf() const { return children.empty(); }
};

// The layout for `count` views, reproducing the shapes Uplink has always used:
// 1 or 2 flat along the main axis, 3 as one full-length view beside a stacked
// pair, 4 as two stacked pairs. `rows` transposes all of them 90°.
//
// The root is always a split so the caller can hand its children straight to
// the existing top-level splitter. A count of 0 gives a childless root.
PaneNode buildShapeTree(int count, bool rows);
