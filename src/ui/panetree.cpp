#include "ui/panetree.h"

static PaneNode leaf(int slot)
{
    PaneNode n;
    n.slot = slot;
    return n;
}

static PaneNode split(Qt::Orientation axis, std::vector<PaneNode> children)
{
    PaneNode n;
    n.axis     = axis;
    n.children = std::move(children);
    return n;
}

PaneNode buildShapeTree(int count, bool rows)
{
    const Qt::Orientation mainAxis  = rows ? Qt::Vertical   : Qt::Horizontal;
    const Qt::Orientation crossAxis = rows ? Qt::Horizontal : Qt::Vertical;

    if (count <= 0)
        return split(mainAxis, {});
    if (count <= 2) {
        // Flat along the main axis
        std::vector<PaneNode> kids;
        for (int i = 0; i < count; i++)
            kids.push_back(leaf(i));
        return split(mainAxis, std::move(kids));
    }
    if (count == 3) {
        // First slot runs the full length, the other two stack beside it
        return split(mainAxis, { leaf(0),
                                 split(crossAxis, { leaf(1), leaf(2) }) });
    }
    // 4 and up: two stacked pairs. Counts above 4 can't happen while the pane
    // cap is 4, and the extra views would be dropped — buildShapeTree is the
    // parity shape, not the general one.
    return split(mainAxis, { split(crossAxis, { leaf(0), leaf(1) }),
                             split(crossAxis, { leaf(2), leaf(3) }) });
}

// ---------------------------------------------------------------------------
// Tree edits
// ---------------------------------------------------------------------------

bool operator==(const PaneNode &a, const PaneNode &b)
{
    if (a.isLeaf() != b.isLeaf()) return false;
    if (a.isLeaf()) return a.slot == b.slot;
    if (a.axis != b.axis || a.children.size() != b.children.size()) return false;
    for (size_t i = 0; i < a.children.size(); ++i)
        if (!(a.children[i] == b.children[i])) return false;
    return true;
}

std::vector<int> leafOrder(const PaneNode &root)
{
    // A root emptied by removeLeaf carries no id — don't report a phantom view.
    if (root.isLeaf()) return root.slot >= 0 ? std::vector<int>{root.slot}
                                             : std::vector<int>{};
    std::vector<int> out;
    if (root.isLeaf()) return out; // unreachable, kept for the reader
    for (const PaneNode &c : root.children) {
        const std::vector<int> sub = leafOrder(c);
        out.insert(out.end(), sub.begin(), sub.end());
    }
    return out;
}

bool containsLeaf(const PaneNode &root, int id)
{
    if (root.isLeaf()) return root.slot == id;
    for (const PaneNode &c : root.children)
        if (containsLeaf(c, id)) return true;
    return false;
}

// A split holding one child becomes that child, and a split directly inside a
// split of the same axis is spliced into its parent. Without the second rule a
// third view dropped below a pair would nest instead of joining the row.
static void normalize(PaneNode &node)
{
    if (node.isLeaf()) return;

    for (PaneNode &c : node.children)
        normalize(c);

    std::vector<PaneNode> merged;
    for (PaneNode &c : node.children) {
        if (!c.isLeaf() && c.axis == node.axis) {
            for (PaneNode &grand : c.children)
                merged.push_back(std::move(grand));
        } else {
            merged.push_back(std::move(c));
        }
    }
    node.children = std::move(merged);

    if (node.children.size() == 1) {
        PaneNode only = std::move(node.children[0]);
        node = std::move(only);
    }
}

// Replaces the leaf holding `id` with `replacement`, in place.
static bool replaceLeaf(PaneNode &node, int id, PaneNode replacement)
{
    if (node.isLeaf()) {
        if (node.slot != id) return false;
        node = std::move(replacement);
        return true;
    }
    for (PaneNode &c : node.children)
        if (replaceLeaf(c, id, replacement)) return true;
    return false;
}

bool splitLeaf(PaneNode &root, int targetId, int newId, Qt::Orientation axis, bool before)
{
    if (!containsLeaf(root, targetId)) return false;

    PaneNode target;
    target.slot = targetId;
    PaneNode added;
    added.slot = newId;

    PaneNode replacement;
    replacement.axis = axis;
    if (before) replacement.children = { std::move(added), std::move(target) };
    else        replacement.children = { std::move(target), std::move(added) };

    if (!replaceLeaf(root, targetId, std::move(replacement))) return false;
    normalize(root);
    return true;
}

static void swapIds(PaneNode &node, int a, int b)
{
    if (node.isLeaf()) {
        if (node.slot == a)      node.slot = b;
        else if (node.slot == b) node.slot = a;
        return;
    }
    for (PaneNode &c : node.children)
        swapIds(c, a, b);
}

bool swapLeaves(PaneNode &root, int a, int b)
{
    if (a == b || !containsLeaf(root, a) || !containsLeaf(root, b)) return false;
    swapIds(root, a, b);
    return true;
}

static bool eraseLeaf(PaneNode &node, int id)
{
    if (node.isLeaf()) return false;
    for (auto it = node.children.begin(); it != node.children.end(); ++it) {
        if (it->isLeaf() && it->slot == id) {
            node.children.erase(it);
            return true;
        }
        if (eraseLeaf(*it, id)) return true;
    }
    return false;
}

bool removeLeaf(PaneNode &root, int id)
{
    // A lone leaf at the root has no parent to erase it from; leave the caller
    // an empty root rather than a tree still claiming the view.
    if (root.isLeaf()) {
        if (root.slot != id) return false;
        root = PaneNode{};
        root.slot = -1;
        return true;
    }
    if (!eraseLeaf(root, id)) return false;
    normalize(root);
    return true;
}

bool moveLeafBeside(PaneNode &root, int movingId, int targetId,
                    Qt::Orientation axis, bool before)
{
    if (movingId == targetId) return false;
    if (!containsLeaf(root, movingId) || !containsLeaf(root, targetId)) return false;

    PaneNode working = root;
    if (!removeLeaf(working, movingId)) return false;
    // Removing the moving leaf can't have taken the target with it: they are
    // different leaves and only empty splits collapse.
    if (!splitLeaf(working, targetId, movingId, axis, before)) return false;
    root = std::move(working);
    return true;
}

PaneNode flattenTree(const PaneNode &root, Qt::Orientation axis)
{
    PaneNode flat;
    flat.axis = axis;
    for (int id : leafOrder(root)) {
        PaneNode leafNode;
        leafNode.slot = id;
        flat.children.push_back(std::move(leafNode));
    }
    // The root stays a split even with a single child, so callers can always
    // hand its children straight to the top-level splitter.
    return flat;
}
