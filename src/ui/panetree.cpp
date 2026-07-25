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
