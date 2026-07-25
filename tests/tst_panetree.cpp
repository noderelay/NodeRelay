#include <QtTest/QtTest>
#include "ui/panetree.h"

#include <set>

// Collects every leaf slot in the tree, in layout order.
static QList<int> leafSlots(const PaneNode &node)
{
    if (node.isLeaf()) return {node.slot};
    QList<int> out;
    for (const PaneNode &c : node.children)
        out += leafSlots(c);
    return out;
}

static int depth(const PaneNode &node)
{
    if (node.isLeaf()) return 1;
    int deepest = 0;
    for (const PaneNode &c : node.children)
        deepest = qMax(deepest, depth(c));
    return deepest + 1;
}

class TstPaneTree : public QObject
{
    Q_OBJECT

private slots:

    void everyViewAppearsOnce()
    {
        for (int count = 1; count <= 4; ++count) {
            for (bool rows : {false, true}) {
                // "slots" is a Qt keyword, hence the name
                const QList<int> found = leafSlots(buildShapeTree(count, rows));
                QCOMPARE(found.size(), count);
                const std::set<int> unique(found.cbegin(), found.cend());
                QCOMPARE(static_cast<int>(unique.size()), count);
                QCOMPARE(*unique.cbegin(), 0);
                QCOMPARE(*unique.crbegin(), count - 1);
            }
        }
    }

    void rootAxisFollowsRows()
    {
        QCOMPARE(buildShapeTree(2, false).axis, Qt::Horizontal); // columns
        QCOMPARE(buildShapeTree(2, true).axis,  Qt::Vertical);   // rows
    }

    void oneAndTwoAreFlat()
    {
        const PaneNode one = buildShapeTree(1, false);
        QCOMPARE(one.children.size(), size_t(1));
        QVERIFY(one.children[0].isLeaf());

        const PaneNode two = buildShapeTree(2, false);
        QCOMPARE(two.children.size(), size_t(2));
        QVERIFY(two.children[0].isLeaf());
        QVERIFY(two.children[1].isLeaf());
        QCOMPARE(depth(two), 2); // no nesting
    }

    void threeIsOneBesideAStackedPair()
    {
        const PaneNode t = buildShapeTree(3, false);
        QCOMPARE(t.children.size(), size_t(2));
        QVERIFY(t.children[0].isLeaf());
        QCOMPARE(t.children[0].slot, 0);
        QVERIFY(!t.children[1].isLeaf());
        QCOMPARE(t.children[1].children.size(), size_t(2));
        // The pair splits across the main axis
        QCOMPARE(t.axis, Qt::Horizontal);
        QCOMPARE(t.children[1].axis, Qt::Vertical);
    }

    void fourIsTwoStackedPairs()
    {
        const PaneNode t = buildShapeTree(4, false);
        QCOMPARE(t.children.size(), size_t(2));
        for (const PaneNode &half : t.children) {
            QVERIFY(!half.isLeaf());
            QCOMPARE(half.children.size(), size_t(2));
            QCOMPARE(half.axis, Qt::Vertical);
        }
        QCOMPARE(leafSlots(t), (QList<int>{0, 1, 2, 3}));
    }

    void rowsTransposesEveryAxis()
    {
        for (int count : {3, 4}) {
            const PaneNode cols = buildShapeTree(count, false);
            const PaneNode rows = buildShapeTree(count, true);
            QCOMPARE(rows.axis, Qt::Vertical);
            QCOMPARE(cols.axis, Qt::Horizontal);
            // Same shape, opposite axes, same view order
            QCOMPARE(leafSlots(rows), leafSlots(cols));
            QCOMPARE(depth(rows), depth(cols));
            for (size_t i = 0; i < rows.children.size(); ++i)
                if (!rows.children[i].isLeaf())
                    QCOMPARE(rows.children[i].axis, Qt::Horizontal);
        }
    }

    // ── tree edits ──────────────────────────────────────────────────────────

    void splitTurnsALeafIntoAPair()
    {
        PaneNode t = buildShapeTree(1, false);          // just view 0
        QVERIFY(splitLeaf(t, 0, 1, Qt::Horizontal, false));
        QCOMPARE(leafOrder(t), (std::vector<int>{0, 1}));
        QCOMPARE(t.axis, Qt::Horizontal);

        QVERIFY(splitLeaf(t, 0, 2, Qt::Horizontal, true)); // 2 lands before 0
        QCOMPARE(leafOrder(t), (std::vector<int>{2, 0, 1}));
    }

    void splitMissingLeafFails()
    {
        PaneNode t = buildShapeTree(2, false);
        QVERIFY(!splitLeaf(t, 99, 5, Qt::Vertical, false));
        QCOMPARE(leafOrder(t), (std::vector<int>{0, 1}));
    }

    // The whole point of the rewrite: three views stacked equally, which the
    // old count-driven shapes could not express.
    void sameAxisSplitsFlattenIntoOneRow()
    {
        PaneNode t = buildShapeTree(1, false);
        QVERIFY(splitLeaf(t, 0, 1, Qt::Vertical, false));  // 0 above 1
        QVERIFY(splitLeaf(t, 1, 2, Qt::Vertical, false));  // 2 below 1

        QCOMPARE(leafOrder(t), (std::vector<int>{0, 1, 2}));
        QCOMPARE(t.axis, Qt::Vertical);
        QCOMPARE(t.children.size(), size_t(3));            // flat, not nested
        for (const PaneNode &c : t.children)
            QVERIFY(c.isLeaf());
    }

    void crossAxisSplitStaysNested()
    {
        PaneNode t = buildShapeTree(1, false);
        QVERIFY(splitLeaf(t, 0, 1, Qt::Horizontal, false));
        QVERIFY(splitLeaf(t, 1, 2, Qt::Vertical, false));  // different axis
        QCOMPARE(t.children.size(), size_t(2));
        QVERIFY(t.children[0].isLeaf());
        QVERIFY(!t.children[1].isLeaf());
        QCOMPARE(t.children[1].axis, Qt::Vertical);
    }

    void removeCollapsesTheSplitItEmpties()
    {
        PaneNode t = buildShapeTree(3, false);   // 0 beside {1,2}
        QVERIFY(removeLeaf(t, 2));
        QCOMPARE(leafOrder(t), (std::vector<int>{0, 1}));
        QCOMPARE(t.children.size(), size_t(2));  // the pair's split is gone
        QVERIFY(t.children[1].isLeaf());
    }

    void removeUnknownLeafFails()
    {
        PaneNode t = buildShapeTree(2, false);
        QVERIFY(!removeLeaf(t, 7));
        QCOMPARE(leafOrder(t), (std::vector<int>{0, 1}));
    }

    void swapExchangesPositions()
    {
        PaneNode t = buildShapeTree(3, false);
        QVERIFY(swapLeaves(t, 0, 2));
        QCOMPARE(leafOrder(t), (std::vector<int>{2, 1, 0}));
        QVERIFY(!swapLeaves(t, 0, 0));
        QVERIFY(!swapLeaves(t, 0, 42));
    }

    void moveBesideRelocatesWithoutDuplicating()
    {
        PaneNode t = buildShapeTree(3, false);   // 0 beside {1,2}
        QVERIFY(moveLeafBeside(t, 2, 0, Qt::Horizontal, true)); // 2 left of 0
        const std::vector<int> order = leafOrder(t);
        QCOMPARE(order, (std::vector<int>{2, 0, 1}));
        QCOMPARE(order.size(), size_t(3));       // moved, not copied
    }

    void moveOntoItselfFails()
    {
        PaneNode t = buildShapeTree(2, false);
        const PaneNode before = t;
        QVERIFY(!moveLeafBeside(t, 1, 1, Qt::Vertical, false));
        QVERIFY(t == before);
    }

    // A no-op drop: view 1 already sits directly after view 0.
    void moveToWhereItAlreadyIsLeavesTheTreeEqual()
    {
        PaneNode t = buildShapeTree(2, false);
        PaneNode after = t;
        QVERIFY(moveLeafBeside(after, 1, 0, Qt::Horizontal, false));
        QVERIFY(after == t);
    }

    void flattenPutsEveryViewInOneSplit()
    {
        const PaneNode t = buildShapeTree(4, false);      // two stacked pairs
        const PaneNode flat = flattenTree(t, Qt::Vertical);
        QCOMPARE(flat.axis, Qt::Vertical);
        QCOMPARE(flat.children.size(), size_t(4));
        for (const PaneNode &c : flat.children)
            QVERIFY(c.isLeaf());
        QCOMPARE(leafOrder(flat), leafOrder(t));          // order preserved
    }

    // ── sizes ───────────────────────────────────────────────────────────────

    void splitStartsDownTheMiddle()
    {
        PaneNode t = buildShapeTree(1, false);
        QVERIFY(splitLeaf(t, 0, 1, Qt::Horizontal, false));
        QCOMPARE(t.fractions.size(), size_t(2));
        QVERIFY(qAbs(t.fractions[0] - 0.5) < 1e-9);
        QVERIFY(qAbs(t.fractions[1] - 0.5) < 1e-9);
    }

    void removingAViewSharesOutItsSpace()
    {
        PaneNode t = buildShapeTree(1, false);
        QVERIFY(splitLeaf(t, 0, 1, Qt::Horizontal, false));
        QVERIFY(splitLeaf(t, 1, 2, Qt::Horizontal, false));
        t.fractions = {0.5, 0.2, 0.3};

        QVERIFY(removeLeaf(t, 1));
        QCOMPARE(t.children.size(), size_t(2));
        // 0.5 and 0.3 renormalised, keeping their ratio
        QVERIFY(qAbs(t.fractions[0] - 0.625) < 1e-6);
        QVERIFY(qAbs(t.fractions[1] - 0.375) < 1e-6);
        double sum = 0;
        for (double f : t.fractions) sum += f;
        QVERIFY(qAbs(sum - 1.0) < 1e-9);
    }

    void flatteningNestedSharesMultipliesThrough()
    {
        // A wide left view and a right half holding two rows.
        PaneNode t = buildShapeTree(1, false);
        QVERIFY(splitLeaf(t, 0, 1, Qt::Horizontal, false));
        t.fractions = {0.7, 0.3};
        QVERIFY(splitLeaf(t, 1, 2, Qt::Horizontal, false)); // same axis: splices up

        // The right half's 0.3 is divided between the two views that shared it
        QCOMPARE(t.children.size(), size_t(3));
        QVERIFY(qAbs(t.fractions[0] - 0.7)  < 1e-6);
        QVERIFY(qAbs(t.fractions[1] - 0.15) < 1e-6);
        QVERIFY(qAbs(t.fractions[2] - 0.15) < 1e-6);
    }

    void differentSizesMakeTreesUnequal()
    {
        PaneNode a = buildShapeTree(2, false);
        PaneNode b = a;
        a.fractions = {0.5, 0.5};
        b.fractions = {0.8, 0.2};
        QVERIFY(a != b);
        b.fractions = {0.5, 0.5};
        QVERIFY(a == b);
    }

    void flattenResetsToEvenShares()
    {
        PaneNode t = buildShapeTree(2, false);
        t.fractions = {0.9, 0.1};
        const PaneNode flat = flattenTree(t, Qt::Vertical);
        QCOMPARE(flat.fractions.size(), size_t(2));
        QVERIFY(qAbs(flat.fractions[0] - 0.5) < 1e-9);
    }

    void emptyTreeHasNoChildren()
    {
        const PaneNode t = buildShapeTree(0, false);
        QVERIFY(t.children.empty());
    }
};

QTEST_MAIN(TstPaneTree)
#include "tst_panetree.moc"
