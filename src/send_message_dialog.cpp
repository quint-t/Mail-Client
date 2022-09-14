#include <QFileDialog>
#include <QMessageBox>

#include "send_message_dialog.h"
#include "ui_send_message_dialog.h"

SendMessageDialog::SendMessageDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SendMessageDialog)
{
    ui->setupUi(this);
    ui->attachments_table->setSortingEnabled(false);
    ui->attachments_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
}

SendMessageDialog::~SendMessageDialog()
{
    delete ui;
}

mailio::message SendMessageDialog::get_message()
{
    QRegularExpression delim("[,;\t ]+");
    mailio::message message;
    QString subject = ui->subject_line_edit->text();
    QStringList recipients = ui->recipients_line_edit->text().split(delim, Qt::SkipEmptyParts);
    QStringList cc_recipients = ui->cc_recipients_line_edit->text().split(delim, Qt::SkipEmptyParts);
    QStringList bcc_recipients = ui->bcc_recipients_line_edit->text().split(delim, Qt::SkipEmptyParts);
    QString content = ui->content_text_edit->toPlainText();
    message.subject(subject.toStdString());
    for (auto recipient : recipients)
    {
        auto recipient_s = recipient.simplified().toStdString();
        message.add_recipient(mailio::mail_address(recipient_s, recipient_s));
    }
    for (auto recipient : cc_recipients)
    {
        auto recipient_s = recipient.simplified().toStdString();
        message.add_cc_recipient(mailio::mail_address(recipient_s, recipient_s));
    }
    for (auto recipient : bcc_recipients)
    {
        auto recipient_s = recipient.simplified().toStdString();
        message.add_bcc_recipient(mailio::mail_address(recipient_s, recipient_s));
    }
    message.content_transfer_encoding(mailio::mime::content_transfer_encoding_t::BASE_64);
    message.content_type(mailio::message::media_type_t::TEXT, "plain", "utf-8");
    message.content(content.toStdString());
    if (!attachments.empty())
    {
        std::list<std::tuple<std::istream &, std::string, mailio::message::content_type_t>> attachments_tlist;
        for (auto attachment : attachments)
        {
            std::shared_ptr<std::stringstream> ss;
            std::string filename;
            mailio::message::content_type_t content_type;
            std::tie(ss, filename, content_type) = attachment;
            attachments_tlist.emplace_back(
                std::tuple<std::istream &, std::string, mailio::message::content_type_t>(*ss, filename, content_type));
        }
        message.attach(attachments_tlist);
    }
    return message;
}

void SendMessageDialog::on_add_attachment_clicked()
{
    try
    {
        QString filename = QFileDialog::getOpenFileName(this, "Добавить вложение", "", "All files (*.*)");
        if (filename.isEmpty())
        {
            return;
        }
        std::fstream file_stream(filename.toStdWString().c_str(), std::ios::in | std::ios::binary);
        if (!file_stream.is_open())
        {
            QMessageBox::warning(this, "Внимание", "Файл недоступен для чтения");
            return;
        }
        file_stream.seekg(0, file_stream.end);
        std::basic_istream<char>::pos_type size = file_stream.tellg();
        file_stream.seekg(0, file_stream.beg);
        std::string attachment_size;
        constexpr size_t kb = 1 << 10, mb = kb << 10, gb = mb << 10;
        if (size >= gb)
        {
            size = size * 100 / gb;
            attachment_size = std::to_string(size / 100) + "." + std::to_string(size % 100) + " ГиБ";
        }
        else if (size >= mb)
        {
            size = size * 100 / mb;
            attachment_size = std::to_string(size / 100) + "." + std::to_string(size % 100) + " МиБ";
        }
        else if (size >= kb)
        {
            size = size * 100 / kb;
            attachment_size = std::to_string(size / 100) + "." + std::to_string(size % 100) + " КиБ";
        }
        else
        {
            attachment_size = std::to_string(size) + " Б";
        }
        std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>();
        std::ostream &os = *ss;
        os << ((std::istream &)file_stream).rdbuf();
        ss->seekg(0, ss->beg);
        file_stream.close();
        std::string filename_s = filename.toStdString();
        std::string base_filename = filename_s.substr(filename_s.find_last_of("/\\") + 1);
        int new_last_index = ui->attachments_table->rowCount();
        attachments.emplace_back(
            std::tuple<std::shared_ptr<std::stringstream>, std::string, mailio::message::content_type_t>(
                ss, base_filename, mailio::message::content_type_t(mailio::message::media_type_t::MULTIPART, "mixed")));
        ui->attachments_table->setRowCount(new_last_index + 1);
        ui->attachments_table->setItem(new_last_index, 0, new QTableWidgetItem(QString::fromStdString(base_filename)));
        ui->attachments_table->setItem(new_last_index, 1,
                                       new QTableWidgetItem(QString::fromStdString(attachment_size)));
        ui->attachments_table->setItem(new_last_index, 2, new QTableWidgetItem(filename));
        ui->attachments_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Внимание", "Ошибка добавления вложения");
    }
}

void SendMessageDialog::on_remove_attachment_clicked()
{
    try
    {
        if (ui->attachments_table->currentItem() != nullptr)
        {
            int current_row = ui->attachments_table->currentRow();
            auto it = attachments.begin();
            std::advance(it, current_row);
            attachments.erase(it);
            ui->attachments_table->removeRow(current_row);
            ui->attachments_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Внимание", "Ошибка удаления вложения");
    }
}

void SendMessageDialog::closeEvent(QCloseEvent *event)
{
    if (QMessageBox::question(this, "Внимание", "Сообщение не будет отправлено и/или сохранено. Продолжить?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}
