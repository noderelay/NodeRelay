#include "aboutdialog.h"
#include "appicons.h"
#include "gitversion.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>

AboutDialog::AboutDialog(QWidget *parent)
    : QFrame(parent)
{
    setFixedSize(460, 280);
    setFrameStyle(QFrame::Box | QFrame::Raised);
    setLineWidth(1);
    setAutoFillBackground(true);
    hide();

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 120));
    setGraphicsEffect(shadow);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(24, 20, 24, 16);

    auto *logo = new QLabel;
    logo->setPixmap(AppIcons::aboutLogo().scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);
    layout->addWidget(logo);

    auto *version = new QLabel("Uplink  v" UPLINK_VERSION_FULL);
    QFont f = version->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 2);
    version->setFont(f);
    version->setAlignment(Qt::AlignCenter);
    layout->addWidget(version);

    auto *desc = new QLabel(
        "A fast, secure, IRCv3-featured IRC client\n"
        "built with Qt6 and C++"
    );
    desc->setAlignment(Qt::AlignCenter);
    desc->setStyleSheet("color: palette(text);");
    layout->addWidget(desc);

    layout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &AboutDialog::hide);
    layout->addWidget(buttons);
}

void AboutDialog::showCentered()
{
    if (QWidget *p = parentWidget()) {
        move(
            (p->width()  - width())  / 2,
            (p->height() - height()) / 2
        );
    }
    show();
    raise();
    setFocus();
}

void AboutDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Return)
        hide();
    else
        QFrame::keyPressEvent(event);
}
