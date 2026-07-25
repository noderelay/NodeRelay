#include "ui/panetree.h"

#include <QtGlobal>

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

std::vector<double> evenFractions(size_t count)
{
    if (count == 0) return {};
    return std::vector<double>(count, 1.0 / double(count));
}

// Sizes are only meaningful next to the children they describe; anything that
// edits the child list runs through here so a stale array can't survive.
static std::vector<double> fractionsOf(const PaneNode &node)
{
    if (node.fractions.size() == node.children.size()) return node.fractions;
    return evenFractions(node.children.size());
}

static void normalizeFractions(std::vector<double> &f)
{
    double sum = 0;
    for (double v : f) sum += v;
    if (sum <= 0) { f = evenFractions(f.size()); return; }
    for (double &v : f) v /= sum;
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
    const std::vector<double> fa = fractionsOf(a), fb = fractionsOf(b);
    for (size_t i = 0; i < fa.size(); ++i)
        if (qAbs(fa[i] - fb[i]) > 0.001) return false;   // a drag apart, not a rounding apart
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
    std::vector<double>   mergedF;
    const std::vector<double> mine = fractionsOf(node);
    for (size_t i = 0; i < node.children.size(); ++i) {
        PaneNode &c = node.children[i];
        if (!c.isLeaf() && c.axis == node.axis) {
            // Spliced-up children divide the share their parent held, so the
            // views keep the proportions they had on screen.
            const std::vector<double> inner = fractionsOf(c);
            for (size_t j = 0; j < c.children.size(); ++j) {
                merged.push_back(std::move(c.children[j]));
                mergedF.push_back(mine[i] * inner[j]);
            }
        } else {
            merged.push_back(std::move(c));
            mergedF.push_back(mine[i]);
        }
    }
    node.children  = std::move(merged);
    node.fractions = std::move(mergedF);
    normalizeFractions(node.fractions);

    if (node.children.size() == 1) {
        PaneNode only = std::move(node.children[0]);
        node = std::move(only);   // a lone child takes the whole share
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
    replacement.fractions = evenFractions(2);   // a split starts down the middle

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
    std::vector<double> f = fractionsOf(node);
    for (size_t i = 0; i < node.children.size(); ++i) {
        if (node.children[i].isLeaf() && node.children[i].slot == id) {
            node.children.erase(node.children.begin() + long(i));
            f.erase(f.begin() + long(i));
            normalizeFractions(f);  // the freed space goes to the survivors
            node.fractions = std::move(f);
            return true;
        }
        if (eraseLeaf(node.children[i], id)) return true;
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
    // Shares from the old shape mean nothing once everything is one split.
    for (int id : leafOrder(root)) {
        PaneNode leafNode;
        leafNode.slot = id;
        flat.children.push_back(std::move(leafNode));
    }
    flat.fractions = evenFractions(flat.children.size());
    // The root stays a split even with a single child, so callers can always
    // hand its children straight to the top-level splitter.
    return flat;
}
