#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QCloseEvent>
#include <QDialog>

#include "imap_settings.h"
#include "smtp_settings.h"

namespace Ui
{
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    void set_imap_settings(const ImapSettings &arg_imap_settings);
    void set_smtp_settings(const SmtpSettings &arg_smtp_settings);
    ImapSettings get_imap_settings() const;
    SmtpSettings get_smtp_settings() const;
    ~SettingsDialog() override;

  private slots:
    void on_mail_server_combo_box_textActivated(const QString &text);
    void on_smtp_login_line_edit_textEdited(const QString &arg1);

  private:
    void closeEvent(QCloseEvent *event) override;

    Ui::SettingsDialog *ui;
};

#endif // SETTINGS_DIALOG_H
