#pragma once
#include <QFrame>
#include <QVector>

class QLineEdit;
class QScrollArea;
class QWidget;
class QGridLayout;
class QToolButton;

class EmojiPicker : public QFrame
{
    Q_OBJECT
public:
    explicit EmojiPicker(QWidget *parent = nullptr);

    // Show picker with its bottom-right corner at globalAnchor
    void showAt(const QPoint &globalAnchor);

signals:
    void emojiSelected(const QString &emoji);

private slots:
    void onSearch(const QString &text);

private:
    void buildButtons();
    void filterGrid(const QString &filter);

    QLineEdit   *m_search{nullptr};
    QScrollArea *m_scroll{nullptr};
    QWidget     *m_gridWidget{nullptr};
    QGridLayout *m_gridLayout{nullptr};

    struct BtnEntry { QToolButton *btn{nullptr}; QString shortcode; };
    QVector<BtnEntry> m_buttons;
};
