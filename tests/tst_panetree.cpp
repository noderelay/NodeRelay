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

    void emptyTreeHasNoChildren()
    {
        const PaneNode t = buildShapeTree(0, false);
        QVERIFY(t.children.empty());
    }
};

QTEST_MAIN(TstPaneTree)
#include "tst_panetree.moc"
