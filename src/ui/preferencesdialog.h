#pragma once
#include <QDialog>
#include "config/config.h"

class QButtonGroup;
class QCheckBox;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class SolidComboBox;

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(const Config &cfg, QWidget *parent = nullptr);
    void syncFromConfig(const Config &cfg);

signals:
    void themeChanged(const QString &name);
    void themeAutoToggled(bool on);
    void themeLightChanged(const QString &name);
    void themeDarkChanged(const QString &name);
    void fontConfigRequested();
    void appIconChanged(const QString &key);
    void topicBarToggled(bool on);
    void nickPrefixToggled(bool on);
    void emojiBtnToggled(bool on);
    void sendBtnToggled(bool on);
    void typingIndicatorToggled(bool on);
    void notificationsToggled(bool on);
    void coloredNicksToggled(bool on);
    void hangingIndentToggled(bool on);
    void loggingToggled(bool on);
    void linkPreviewsToggled(bool on);
    void avatarsToggled(bool on);
    void unreadCountsToggled(bool on);
    void timestampsToggled(bool on);
    void highlightWordsChanged(const QString &words);
    void nickBracketsChanged(const QString &brackets);
    void paneSplitAutoToggled(bool on);
    void panelCardsToggled(bool on);
    void menuStyleChanged(const QString &style);
    void persistenceChanged(const QString &mode);
    void manageServersRequested();
    void aboutRequested();
    void docsRequested();
    void profileSetRequested(const QString &displayName, const QString &avatarUrl,
                             const QString &statusText);
    void dccAllowLanToggled(bool on);
    void dccExternalIpChanged(const QString &ip);
    void dccPortRangeChanged(quint16 lo, quint16 hi);
    void scriptsChanged(const QList<ScriptBinding> &scripts);

private:
    QWidget *createAppearancePage(const Config &cfg, const QColor &accent);
    QWidget *createChatWindowPage(const Config &cfg);
    QWidget *createInterfacePage(const Config &cfg);
    QWidget *createNotificationsPage(const Config &cfg);
    QWidget *createLoggingPage(const Config &cfg);
    QWidget *createTransfersPage(const Config &cfg);
    QWidget *createProfilePage(const Config &cfg, const QColor &accent);
    QWidget *createScriptsPage(const Config &cfg, const QColor &accent);

    QListWidget    *m_navList{nullptr};
    QStackedWidget *m_pages{nullptr};
    QPushButton    *m_themeBtn{nullptr};   // Appearance page theme-list toggle
    QCheckBox      *m_themeAutoCheck{nullptr};   // absent on Qt < 6.5 builds
    SolidComboBox  *m_themeLightCombo{nullptr};
    SolidComboBox  *m_themeDarkCombo{nullptr};

    QCheckBox *m_topicCheck{nullptr};
    QCheckBox *m_nickPrefixCheck{nullptr};
    QCheckBox *m_emojiCheck{nullptr};
    QCheckBox *m_sendBtnCheck{nullptr};

    QCheckBox *m_panelCardsCheck{nullptr};
    QCheckBox *m_typingCheck{nullptr};
    QCheckBox *m_notificationsCheck{nullptr};
    QCheckBox *m_coloredNicksCheck{nullptr};
    QCheckBox *m_hangingIndentCheck{nullptr};
    QCheckBox *m_loggingCheck{nullptr};
    QCheckBox *m_linkPreviewsCheck{nullptr};
    QCheckBox *m_avatarsCheck{nullptr};
    QCheckBox *m_unreadCountsCheck{nullptr};
    QCheckBox *m_timestampsCheck{nullptr};
    QLineEdit *m_highlightWordsEdit{nullptr};
    QButtonGroup *m_bracketsGroup{nullptr};
    QCheckBox    *m_paneSplitAutoCheck{nullptr};
    QButtonGroup *m_menuStyleGroup{nullptr};
    QButtonGroup *m_persistenceGroup{nullptr};
    QCheckBox *m_dccAllowLanCheck{nullptr};
    QLineEdit *m_dccExternalIpEdit{nullptr};
    QSpinBox  *m_dccPortMinSpin{nullptr};
    QSpinBox  *m_dccPortMaxSpin{nullptr};
    QLineEdit *m_displayNameEdit{nullptr};
    QLineEdit *m_avatarUrlEdit{nullptr};
    QLineEdit *m_statusEdit{nullptr};

    static const QList<QPair<QString,QString>> s_iconChoices;
    static const QList<QPair<QString,QString>> s_bracketChoices;
    static const QList<QPair<QString,QString>> s_menuStyleChoices;
    static const QList<QPair<QString,QString>> s_persistenceChoices;
};
