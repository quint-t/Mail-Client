#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include <QCloseEvent>
#include <QMainWindow>
#include <QTableWidget>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QThread>
#include <QTreeWidget>

#include "imap_settings.h"
#include "smtp_settings.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

class ThreadForManualUpdate; // declaration for thread_for_manual_update attribute in MainWindow class
class ThreadForAutoUpdate;   // declaration for thread_for_update attribute in MainWindow class

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

  private slots:
    void update_folders_and_messages_ui();
    void on_update_folders_and_messages_triggered();
    void on_set_up_account_triggered();
    void on_exit_program_triggered();
    void on_add_folder_triggered();
    void on_rename_folder_triggered();
    void on_delete_selected_folder_triggered();
    void on_send_message_triggered();
    void on_download_attachment_triggered();
    void on_copy_selected_message_triggered();
    void on_delete_selected_message_triggered();
    void on_clear_all_and_disable_autoupdate_triggered();
    void on_show_message_statistics_triggered();
    void on_show_about_window_triggered();
    void on_folders_tree_itemClicked(QTreeWidgetItem *, int);
    void on_messages_table_itemClicked(QTableWidgetItem *);
    void on_search_query_textChanged(const QString &);
    void on_search_query_returnPressed();

  private:
    void update_folders_and_messages();
    void folder_chosen_ui();
    static int get_number_of_folders_recursive(QTreeWidgetItem *twi);
    static void list_folders_recursive(
        QTreeWidgetItem *twi, const mailio::imaps::mailbox_folder_t &folder, mailio::imaps &conn,
        const std::list<std::string> &path, std::unordered_map<std::string, mailio::message> &messages,
        std::unordered_map<std::string, unsigned long long> &messages_sizes,
        std::map<std::list<std::string>, std::unordered_set<std::string>> &mailboxes_to_messages_ids,
        std::unordered_map<std::string, unsigned long> &messages_ids_to_uids,
        std::unordered_map<unsigned long, std::string> &uids_to_messages_ids);
    static std::string get_current_datetime_as_string();
    static std::string local_date_time_as_string(const boost::local_time::local_date_time &ldt);
    void closeEvent(QCloseEvent *event) override;
    void clear_all();

    Ui::MainWindow *ui;
    QTextDocument empty_text_document;
    ImapSettings imap_settings;
    SmtpSettings smtp_settings;
    ThreadForManualUpdate *thread_for_manual_update;
    ThreadForAutoUpdate *thread_for_auto_update;
    std::mutex main_mutex;
    QList<QTreeWidgetItem *> folders;
    std::list<std::string> current_item;
    std::unordered_map<std::string, unsigned long long> messages_sizes;
    std::unordered_map<std::string, mailio::message> messages;
    std::unordered_map<std::string, unsigned long> messages_ids_to_uids;
    std::unordered_map<std::string, QTextDocument> messages_ids_to_text_documents;
    std::unordered_map<unsigned long, std::string> uids_to_messages_ids;
    std::map<std::list<std::string>, std::unordered_set<std::string>> mailboxes_to_messages_ids;
    std::chrono::system_clock::time_point start;
    const std::chrono::duration<long long int> timeout = std::chrono::seconds(5000);
    bool update_status;
    std::string update_status_message;
    friend class ThreadForManualUpdate;
    friend class ThreadForAutoUpdate;
};

class ThreadForManualUpdate : public QThread
{
    Q_OBJECT

  public:
    explicit ThreadForManualUpdate(MainWindow *mainwindow) : mainwindow(mainwindow)
    {
        if (!mainwindow)
        {
            throw std::runtime_error("mainwindow is null");
        }
    }
    ~ThreadForManualUpdate()
    {
    }

    void run()
    {
        std::lock_guard<std::mutex> main_lock(mainwindow->main_mutex);
        mainwindow->update_folders_and_messages();
        emit folders_and_messages_updated();
    }

  private:
    MainWindow *mainwindow;
  signals:
    void folders_and_messages_updated();
};

class ThreadForAutoUpdate : public QThread
{
    Q_OBJECT

  public:
    explicit ThreadForAutoUpdate(MainWindow *mainwindow, const ImapSettings *imap_settings)
        : mainwindow(mainwindow), imap_settings(imap_settings)
    {
        if (!mainwindow)
        {
            throw std::runtime_error("mainwindow is null");
        }
        if (!imap_settings)
        {
            throw std::runtime_error("imap_settings is null");
        }
    }
    ~ThreadForAutoUpdate()
    {
    }

    void run()
    {
        this->current_second = 1;
        while (true)
        {
            if (imap_settings->get_autoupdate())
            {
                if (this->current_second < imap_settings->get_autoupdate_interval())
                {
                    this->current_second += 1;
                }
                else
                {
                    this->current_second = 1;
                    std::lock_guard<std::mutex> main_lock(mainwindow->main_mutex);
                    mainwindow->update_folders_and_messages();
                    emit folders_and_messages_updated();
                }
            }
            else
            {
                this->current_second = 1;
            }
            QThread::sleep(1);
        }
    }

  private:
    MainWindow *mainwindow;
    const ImapSettings *imap_settings;
    int current_second;
  signals:
    void folders_and_messages_updated();
};

#endif // MAINWINDOW_H
