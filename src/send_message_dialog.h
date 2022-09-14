#ifndef SEND_MESSAGE_DIALOG_H
#define SEND_MESSAGE_DIALOG_H

#include <QCloseEvent>
#include <QDialog>

#include <mailio/message.hpp>

namespace Ui
{
class SendMessageDialog;
}

class SendMessageDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SendMessageDialog(QWidget *parent = nullptr);
    mailio::message get_message();
    ~SendMessageDialog() override;

  private slots:
    void on_add_attachment_clicked();
    void on_remove_attachment_clicked();

  private:
    void closeEvent(QCloseEvent *event) override;

    Ui::SendMessageDialog *ui;
    std::list<std::tuple<std::shared_ptr<std::stringstream>, std::string, mailio::message::content_type_t>> attachments;
};

#endif // SEND_MESSAGE_DIALOG_H
