#include <QMessageBox>

#include "settings_dialog.h"
#include "ui_settings_dialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    this->setFixedSize(QSize(540, 360));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::set_imap_settings(const ImapSettings &arg_imap_settings)
{
    ui->imap_server_line_edit->setText(QString::fromStdString(arg_imap_settings.get_server_address()));
    ui->imap_port_spin_box->setValue(arg_imap_settings.get_server_port());
    ui->imap_login_line_edit->setText(QString::fromStdString(arg_imap_settings.get_login()));
    ui->imap_password_line_edit->setText(QString::fromStdString(arg_imap_settings.get_password()));
    ui->autoupdate_check_box->setChecked(arg_imap_settings.get_autoupdate());
    ui->autoupdate_spin_box->setValue(arg_imap_settings.get_autoupdate_interval());
    mailio::imaps::auth_method_t method = arg_imap_settings.get_auth_method();
    if (method == mailio::imaps::auth_method_t::LOGIN)
    {
        ui->imap_auth_combo_box->setCurrentText("LOGIN");
    }
    else if (method == mailio::imaps::auth_method_t::START_TLS)
    {
        ui->imap_auth_combo_box->setCurrentText("START_TLS");
    }
    else
    {
        ui->imap_auth_combo_box->setCurrentText("");
    }
    if (ui->imap_server_line_edit->text().contains("yandex", Qt::CaseInsensitive))
    {
        ui->mail_server_combo_box->setCurrentText("Yandex");
    }
    else if (ui->imap_server_line_edit->text().contains("gmail", Qt::CaseInsensitive))
    {
        ui->mail_server_combo_box->setCurrentText("Gmail");
    }
    else if (ui->imap_server_line_edit->text().contains("mail.ru", Qt::CaseInsensitive))
    {
        ui->mail_server_combo_box->setCurrentText("Mail.ru");
    }
    else
    {
        ui->mail_server_combo_box->clearEditText();
    }
}

void SettingsDialog::set_smtp_settings(const SmtpSettings &arg_smtp_settings)
{
    ui->smtp_server_line_edit->setText(QString::fromStdString(arg_smtp_settings.get_server_address()));
    ui->smtp_port_spin_box->setValue(arg_smtp_settings.get_server_port());
    ui->smtp_login_line_edit->setText(QString::fromStdString(arg_smtp_settings.get_login()));
    ui->smtp_password_line_edit->setText(QString::fromStdString(arg_smtp_settings.get_password()));
    ui->smtp_name_line_edit->setText(QString::fromStdString(arg_smtp_settings.get_name()));
    mailio::smtps::auth_method_t method = arg_smtp_settings.get_auth_method();
    if (method == mailio::smtps::auth_method_t::LOGIN)
    {
        ui->smtp_auth_combo_box->setCurrentText("LOGIN");
    }
    else if (method == mailio::smtps::auth_method_t::START_TLS)
    {
        ui->smtp_auth_combo_box->setCurrentText("START_TLS");
    }
    else if (method == mailio::smtps::auth_method_t::NONE)
    {
        ui->smtp_auth_combo_box->setCurrentText("NONE");
    }
    else
    {
        ui->smtp_auth_combo_box->setCurrentText("");
    }
    if (ui->mail_server_combo_box->currentText().isEmpty())
    {
        if (ui->smtp_server_line_edit->text().contains("yandex", Qt::CaseInsensitive))
        {
            ui->mail_server_combo_box->setCurrentText("Yandex");
        }
        else if (ui->smtp_server_line_edit->text().contains("gmail", Qt::CaseInsensitive))
        {
            ui->mail_server_combo_box->setCurrentText("Gmail");
        }
        else if (ui->smtp_server_line_edit->text().contains("mail.ru", Qt::CaseInsensitive))
        {
            ui->mail_server_combo_box->setCurrentText("Mail.ru");
        }
    }
}

ImapSettings SettingsDialog::get_imap_settings() const
{
    ImapSettings new_imap_settings;
    new_imap_settings.set_server_address(ui->imap_server_line_edit->text().toStdString());
    new_imap_settings.set_server_port(ui->imap_port_spin_box->value());
    new_imap_settings.set_login(ui->imap_login_line_edit->text().toStdString());
    new_imap_settings.set_password(ui->imap_password_line_edit->text().toStdString());
    new_imap_settings.set_autoupdate(ui->autoupdate_check_box->isChecked());
    new_imap_settings.set_autoupdate_interval(ui->autoupdate_spin_box->value());
    if (ui->imap_auth_combo_box->currentText() == "LOGIN")
    {
        new_imap_settings.set_auth_method(mailio::imaps::auth_method_t::LOGIN);
    }
    else if (ui->imap_auth_combo_box->currentText() == "START_TLS")
    {
        new_imap_settings.set_auth_method(mailio::imaps::auth_method_t::START_TLS);
    }
    else
    {
        new_imap_settings.set_auth_method(mailio::imaps::auth_method_t::LOGIN);
    }
    return new_imap_settings;
}

SmtpSettings SettingsDialog::get_smtp_settings() const
{
    SmtpSettings new_smtp_settings;
    new_smtp_settings.set_server_address(ui->smtp_server_line_edit->text().toStdString());
    new_smtp_settings.set_server_port(ui->smtp_port_spin_box->value());
    new_smtp_settings.set_login(ui->smtp_login_line_edit->text().toStdString());
    new_smtp_settings.set_password(ui->smtp_password_line_edit->text().toStdString());
    new_smtp_settings.set_name(ui->smtp_name_line_edit->text().toStdString());
    if (ui->smtp_auth_combo_box->currentText() == "LOGIN")
    {
        new_smtp_settings.set_auth_method(mailio::smtps::auth_method_t::LOGIN);
    }
    else if (ui->smtp_auth_combo_box->currentText() == "START_TLS")
    {
        new_smtp_settings.set_auth_method(mailio::smtps::auth_method_t::START_TLS);
    }
    else if (ui->smtp_auth_combo_box->currentText() == "NONE")
    {
        new_smtp_settings.set_auth_method(mailio::smtps::auth_method_t::NONE);
    }
    else
    {
        new_smtp_settings.set_auth_method(mailio::smtps::auth_method_t::LOGIN);
    }
    return new_smtp_settings;
}

void SettingsDialog::on_mail_server_combo_box_textActivated(const QString &text)
{
    if (text == "Yandex")
    {
        // imap
        ui->imap_server_line_edit->setText("imap.yandex.ru");
        ui->imap_port_spin_box->setValue(993);
        std::string imap_login = ui->imap_login_line_edit->text().toStdString();
        imap_login = imap_login.substr(0, imap_login.find('@')) + "@yandex.ru";
        ui->imap_login_line_edit->setText(QString::fromStdString(imap_login));
        ui->imap_auth_combo_box->setCurrentText("LOGIN");
        // smtp
        ui->smtp_server_line_edit->setText("smtp.yandex.ru");
        ui->smtp_port_spin_box->setValue(465);
        std::string smtp_login = ui->smtp_login_line_edit->text().toStdString();
        smtp_login = smtp_login.substr(0, smtp_login.find('@')) + "@yandex.ru";
        ui->smtp_login_line_edit->setText(QString::fromStdString(smtp_login));
        ui->smtp_auth_combo_box->setCurrentText("LOGIN");
    }
    else if (text == "Gmail")
    {
        // imap
        ui->imap_server_line_edit->setText("imap.gmail.com");
        ui->imap_port_spin_box->setValue(993);
        std::string imap_login = ui->imap_login_line_edit->text().toStdString();
        imap_login = imap_login.substr(0, imap_login.find('@')) + "@gmail.com";
        ui->imap_login_line_edit->setText(QString::fromStdString(imap_login));
        ui->imap_auth_combo_box->setCurrentText("LOGIN");
        // smtp
        ui->smtp_server_line_edit->setText("smtp.gmail.com");
        ui->smtp_port_spin_box->setValue(465);
        std::string smtp_login = ui->smtp_login_line_edit->text().toStdString();
        smtp_login = smtp_login.substr(0, smtp_login.find('@')) + "@gmail.com";
        ui->smtp_login_line_edit->setText(QString::fromStdString(smtp_login));
        ui->smtp_auth_combo_box->setCurrentText("LOGIN");
    }
    else if (text == "Mail.ru")
    {
        // imap
        ui->imap_server_line_edit->setText("imap.mail.ru");
        ui->imap_port_spin_box->setValue(993);
        std::string imap_login = ui->imap_login_line_edit->text().toStdString();
        imap_login = imap_login.substr(0, imap_login.find('@')) + "@mail.ru";
        ui->imap_login_line_edit->setText(QString::fromStdString(imap_login));
        ui->imap_auth_combo_box->setCurrentText("LOGIN");
        // smtp
        ui->smtp_server_line_edit->setText("smtp.mail.ru");
        ui->smtp_port_spin_box->setValue(465);
        std::string smtp_login = ui->smtp_login_line_edit->text().toStdString();
        smtp_login = smtp_login.substr(0, smtp_login.find('@')) + "@mail.ru";
        ui->smtp_login_line_edit->setText(QString::fromStdString(smtp_login));
        ui->smtp_auth_combo_box->setCurrentText("LOGIN");
    }
}

void SettingsDialog::on_smtp_login_line_edit_textEdited(const QString &login)
{
    std::string s = login.toStdString();
    s = s.substr(0, s.find('@'));
    ui->smtp_name_line_edit->setText(QString::fromStdString(s));
}

void SettingsDialog::closeEvent(QCloseEvent *event)
{
    if (QMessageBox::question(this, "Внимание", "Настройки не будут сохранены. Продолжить?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
