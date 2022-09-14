#include <QInputDialog>
#include <QMessageBox>

#include "mainwindow.h"
#include "send_message_dialog.h"
#include "settings_dialog.h"
#include "tools.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->folders_tree->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    ui->folders_tree->setSortingEnabled(true);
    ui->folders_tree->sortByColumn(0, Qt::AscendingOrder);
    ui->messages_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    ui->messages_table->setSortingEnabled(true);
    ui->messages_table->sortByColumn(2, Qt::DescendingOrder);
    // imap initial settings
    imap_settings.set_server_address("imap.yandex.ru");
    imap_settings.set_server_port(993);
    imap_settings.set_login("robert.wayne.alx@yandex.ru");
    imap_settings.set_password("Robw12345");
    imap_settings.set_autoupdate(false);
    imap_settings.set_autoupdate_interval(10);
    imap_settings.set_auth_method(mailio::imaps::auth_method_t::LOGIN);
    // smtp initial settings
    smtp_settings.set_server_address("smtp.yandex.ru");
    smtp_settings.set_server_port(465);
    smtp_settings.set_login("robert.wayne.alx@yandex.ru");
    smtp_settings.set_password("Robw12345");
    smtp_settings.set_name("robert.wayne.alx");
    smtp_settings.set_auth_method(mailio::smtps::auth_method_t::LOGIN);
    // other
    ui->messages_table->hideColumn(ui->messages_table->columnCount() - 1);
    ui->folders_tree->hideColumn(ui->folders_tree->columnCount() - 1);
    ui->horizontal_splitter->setStretchFactor(0, 0);
    ui->horizontal_splitter->setStretchFactor(1, 1);
    this->update_status = true;
    // threads for update folders and messages
    thread_for_auto_update = new ThreadForAutoUpdate(this, &imap_settings);
    connect(thread_for_auto_update, SIGNAL(folders_and_messages_updated()), this,
            SLOT(update_folders_and_messages_ui()));
    thread_for_manual_update = new ThreadForManualUpdate(this);
    connect(thread_for_manual_update, SIGNAL(folders_and_messages_updated()), this,
            SLOT(update_folders_and_messages_ui()));
    thread_for_auto_update->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::update_folders_and_messages()
{
    try
    {
        this->start = std::chrono::system_clock::now();
        mailio::imaps conn = mailio::imaps(imap_settings.get_server_address(),
                                           static_cast<unsigned>(imap_settings.get_server_port()), timeout);
        conn.authenticate(imap_settings.get_login(), imap_settings.get_password(), imap_settings.get_auth_method());
        mailio::imaps::mailbox_folder_t mailbox_main_folder = conn.list_folders("");
        std::unordered_map<std::string, unsigned long> new_messages_ids_to_uids;
        std::unordered_map<unsigned long, std::string> new_uids_to_messages_ids;
        std::map<std::list<std::string>, std::unordered_set<std::string>> new_mailboxes_to_messages_ids;
        QTreeWidgetItem root;
        list_folders_recursive(&root, mailbox_main_folder, conn, std::list<std::string>(), this->messages,
                               this->messages_sizes, new_mailboxes_to_messages_ids, new_messages_ids_to_uids,
                               new_uids_to_messages_ids);
        QList<QTreeWidgetItem *> new_folders;
        for (size_t i = 0, n = root.childCount(); i < n; ++i)
        {
            new_folders.emplace_back(root.child(i)->clone());
        }
        this->messages_ids_to_uids = new_messages_ids_to_uids;
        this->uids_to_messages_ids = new_uids_to_messages_ids;
        this->mailboxes_to_messages_ids = new_mailboxes_to_messages_ids;
        this->folders.swap(new_folders);
        this->update_status = true;
        this->update_status_message.clear();
    }
    catch (const std::exception &exc)
    {
        this->update_status = false;
        this->update_status_message = "Список папок с сообщениями не обновлен: " + std::string(exc.what());
    }
}

void MainWindow::list_folders_recursive(
    QTreeWidgetItem *twi, const mailio::imaps::mailbox_folder_t &folder, mailio::imaps &conn,
    const std::list<std::string> &path, std::unordered_map<std::string, mailio::message> &messages,
    std::unordered_map<std::string, unsigned long long> &messages_sizes,
    std::map<std::list<std::string>, std::unordered_set<std::string>> &mailboxes_to_messages_ids,
    std::unordered_map<std::string, unsigned long> &messages_ids_to_uids,
    std::unordered_map<unsigned long, std::string> &uids_to_messages_ids)
{
    for (auto &folder : folder.folders)
    {
        try
        {
            std::list<std::string> new_list(path);
            new_list.emplace_back(folder.first);
            mailboxes_to_messages_ids[new_list];
            QTreeWidgetItem *child = new QTreeWidgetItem();
            twi->addChild(child);
            child->setText(0, utf7decode(QString::fromStdString(folder.first)));
            child->setText(3, QString::fromStdString(folder.first));
            if (!folder.second.folders.empty())
            {
                list_folders_recursive(child, folder.second, conn, new_list, messages, messages_sizes,
                                       mailboxes_to_messages_ids, messages_ids_to_uids, uids_to_messages_ids);
            }
            mailio::imaps::mailbox_stat_t stat;
            try
            {
                stat = conn.select(new_list, true);
            }
            catch (...)
            {
                stat.messages_unseen = 0;
                stat.messages_no = 0;
            }
            child->setText(1, QString::number(stat.messages_unseen));
            child->setText(2, QString::number(stat.messages_no));
            std::list<mailio::imaps::messages_range_t> ranges;
            ranges.emplace_back(mailio::imaps::messages_range_t(1, std::optional<unsigned long>(-1)));
            std::map<unsigned long, mailio::message> found_messages;
            try
            {
                conn.fetch(ranges, found_messages, true, true, mailio::codec::line_len_policy_t::VERYLARGE);
            }
            catch (...)
            {
                child->setText(1, "-");
                child->setText(2, "-");
                continue;
            }
            if (found_messages.empty())
            {
                child->setText(1, "0");
                child->setText(2, "0");
                continue;
            }
            for (auto message_it = found_messages.begin(); message_it != found_messages.end(); ++message_it)
            {
                try
                {
                    unsigned long uid = message_it->first;
                    mailio::message current_message = message_it->second;
                    std::string message_id = current_message.message_id();
                    messages_ids_to_uids[message_id] = uid;
                    uids_to_messages_ids[uid] = message_id;
                    mailboxes_to_messages_ids[new_list].insert(message_id);
                    if (messages.find(message_id) == messages.end())
                    {
                        // update if empty
                        messages[message_id] = current_message;
                        std::string message_source;
                        current_message.format(message_source);
                        messages_sizes[message_id] = message_source.size();
                        // try to get full message (may except)
                        try
                        {
                            conn.fetch(uid, current_message, true, false);
                            current_message.format(message_source, false);
                            QString qmessage_source = QString::fromStdString(message_source);
                            qmessage_source.replace("This is a MIME-encapsulated message.", "");
                            message_source = qmessage_source.toStdString();
                            messages[message_id].parse(message_source, false);
                            messages_sizes[message_id] = message_source.size();
                            if (current_message.parts().empty())
                            {
                                std::string::size_type pos = message_source.find("\r\n\r\n");
                                if (pos == std::string::npos)
                                {
                                    continue;
                                }
                                std::string content = message_source.substr(pos + 4, std::string::npos);
                                messages[message_id].content(content);
                            }
                        }
                        catch (...)
                        {
                            // try another way
                            std::string new_message_id(message_id);
                            while (new_message_id.size() > 2)
                            {
                                new_message_id.assign(new_message_id.begin() + 1, new_message_id.end() - 1);
                                if (messages.find(new_message_id) != messages.end())
                                {
                                    messages[new_message_id].format(message_source, false);
                                    messages[message_id].parse(message_source, false);
                                    messages_sizes[message_id] = message_source.size();
                                    break;
                                }
                            }
                        }
                    }
                }
                catch (...)
                {
                }
            }
        }
        catch (...)
        {
        }
    }
}

void MainWindow::update_folders_and_messages_ui()
{
    if (!this->update_status)
    {
        std::string current_datetime_s = get_current_datetime_as_string();
        QString log_message = QString::fromStdString(current_datetime_s + ". " + this->update_status_message);
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
        return;
    }
    try
    {
        std::string update_message;
        bool ok = false;
        try
        {
            ui->folders_tree->clear();
            ui->folders_tree->addTopLevelItems(this->folders);
            ui->folders_tree->expandAll();
            int width = 0;
            for (int i = 0, n = ui->folders_tree->columnCount(); i < n; ++i)
            {
                width += ui->folders_tree->columnWidth(i);
            }
            ui->horizontal_splitter->setSizes(QList<int>{width + 10, ui->centralwidget->width() - width - 10});
            std::chrono::duration<double> sec = std::chrono::system_clock::now() - this->start;
            update_message = "Список папок с сообщениями обновлен, сообщения с вложениями доступны оффлайн (" +
                             std::to_string(sec.count()) + " сек.)";
            ok = true;
        }
        catch (const std::exception &exc)
        {
            update_message = "Список папок с сообщениями не обновлен: " + std::string(exc.what());
        }
        std::string current_datetime_s = get_current_datetime_as_string();
        QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
        if (ok)
        {
            folder_chosen_ui();
        }
    }
    catch (const std::exception &exc)
    {
        QString log_message =
            QString::fromStdString("Ошибка обновления списка папок с сообщениями: " + std::string(exc.what()));
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
    }
}

void MainWindow::folder_chosen_ui()
{
    try
    {
        QTreeWidgetItem *current_folder = ui->folders_tree->currentItem();
        std::list<std::string> folder_name;
        if (current_folder == nullptr)
        {
            if (this->current_item.empty())
            {
                return;
            }
            folder_name = this->current_item;
        }
        else
        {
            QTreeWidgetItem *current = current_folder;
            while (current)
            {
                folder_name.emplace_front(current->text(3).toStdString());
                current = current->parent();
            }
            this->current_item = folder_name;
        }
        unsigned long index = 0, n_messages = 0;
        std::list<std::tuple<unsigned long, unsigned long, QTableWidgetItem *>> cells;
        std::unordered_set<std::string> current_messages = this->mailboxes_to_messages_ids[folder_name];
        constexpr unsigned long long kb = 1 << 10, mb = kb << 10, gb = mb << 10;
        for (auto it = current_messages.begin(); it != current_messages.end(); ++it, ++index)
        {
            try
            {
                unsigned long uid = this->messages_ids_to_uids[*it];
                mailio::message current_message = this->messages[*it];
                std::string message_id = current_message.message_id();
                QTableWidgetItem *r0item, *r1item, *r2item, *r3item, *r4item, *r5item, *r6item, *r7item;
                try
                {
                    r0item = new QTableWidgetItem(QString::fromStdString(current_message.subject()).simplified());
                }
                catch (...)
                {
                    r0item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 0, r0item));
                ++n_messages;
                try
                {
                    r1item =
                        new QTableWidgetItem(QString::fromStdString(current_message.from_to_string()).simplified());
                }
                catch (...)
                {
                    r1item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 1, r1item));
                try
                {
                    r2item = new QTableWidgetItem(
                        QString::fromStdString(local_date_time_as_string(current_message.date_time())));
                }
                catch (...)
                {
                    r2item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 2, r2item));
                try
                {
                    r3item = new QTableWidgetItem(
                        QString::fromStdString(current_message.recipients_to_string()).simplified());
                }
                catch (...)
                {
                    r3item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 3, r3item));
                try
                {
                    r4item = new QTableWidgetItem(QString::number(current_message.attachments_size()));
                }
                catch (...)
                {
                    r4item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 4, r4item));
                try
                {
                    unsigned long long size = this->messages_sizes[message_id];
                    std::string size_s;
                    if (size >= gb)
                    {
                        size = size * 100 / gb;
                        size_s = std::to_string(size / 100) + "." + std::to_string(size % 100) + " ГиБ";
                    }
                    else if (size >= mb)
                    {
                        size = size * 100 / mb;
                        size_s = std::to_string(size / 100) + "." + std::to_string(size % 100) + " МиБ";
                    }
                    else if (size >= kb)
                    {
                        size = size * 100 / kb;
                        size_s = std::to_string(size / 100) + "." + std::to_string(size % 100) + " КиБ";
                    }
                    else
                    {
                        size_s = std::to_string(size) + " Б";
                    }
                    r5item = new QTableWidgetItem(QString::fromStdString(size_s));
                }
                catch (...)
                {
                    r5item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 5, r5item));
                try
                {
                    r6item = new QTableWidgetItem(QString::number(uid));
                }
                catch (...)
                {
                    r6item = new QTableWidgetItem("[undefined]");
                }
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 6, r6item));
                r7item = new QTableWidgetItem(QString::fromStdString(*it));
                cells.emplace_back(std::tuple<unsigned long, unsigned long, QTableWidgetItem *>(index, 7, r7item));
                if (this->messages_ids_to_text_documents.find(message_id) == this->messages_ids_to_text_documents.end())
                {
                    QString all_parts;
                    std::vector<mailio::mime> mimes = current_message.parts();
                    if (mimes.empty())
                    {
                        all_parts += QString::fromStdString(current_message.content());
                    }
                    for (const mailio::mime &mime : mimes)
                    {
                        mailio::mime::content_disposition_t content_disposition(mime.content_disposition());
                        if (content_disposition != mailio::mime::content_disposition_t::ATTACHMENT)
                        {
                            mailio::message::content_type_t content_type(mime.content_type());
                            if (content_type.type == mailio::mime::media_type_t::TEXT && content_type.subtype == "html")
                            {
                                all_parts += QString::fromStdString(mime.content());
                            }
                            else
                            {
                                all_parts +=
                                    QTextDocumentFragment::fromPlainText(QString::fromStdString(mime.content()))
                                        .toHtml();
                            }
                        }
                    }
                    this->messages_ids_to_text_documents[message_id].setHtml(all_parts);
                }
            }
            catch (const std::exception &exc)
            {
                ui->logs_text_edit->appendPlainText(
                    QString::fromStdString("Ошибка чтения сообщения " + *it + ": " + std::string(exc.what())));
            }
        }
        ui->messages_table->setSortingEnabled(false);
        ui->messages_table->setRowCount(0);
        ui->messages_table->setRowCount(n_messages);
        unsigned long row, col;
        QTableWidgetItem *item;
        for (auto cell : cells)
        {
            std::tie(row, col, item) = cell;
            ui->messages_table->setItem(row, col, item);
        }
        ui->messages_table->horizontalHeader()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
        ui->messages_table->setSortingEnabled(true);
    }
    catch (const std::exception &exc)
    {
        QString log_message =
            QString::fromStdString("Ошибка чтения сообщений выбранной папки: " + std::string(exc.what()));
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
    }
}

void MainWindow::on_update_folders_and_messages_triggered()
{
    if (!thread_for_manual_update->isRunning())
    {
        thread_for_manual_update->start();
    }
}

void MainWindow::on_set_up_account_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        SettingsDialog new_settings_dialog;
        new_settings_dialog.set_imap_settings(imap_settings);
        new_settings_dialog.set_smtp_settings(smtp_settings);
        if (new_settings_dialog.exec() == QDialog::Accepted)
        {
            ImapSettings new_imap_settings = new_settings_dialog.get_imap_settings();
            SmtpSettings new_smtp_settings = new_settings_dialog.get_smtp_settings();
            imap_settings = new_imap_settings;
            smtp_settings = new_smtp_settings;
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_exit_program_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        if (QMessageBox::question(this, "Внимание", "Вы уверены, что хотите выйти?", QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) == QMessageBox::Yes)
        {
            clear_all();
            exit(0);
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_add_folder_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        std::list<std::string> folder_name_decoded;
        QTreeWidgetItem *current = ui->folders_tree->currentItem();
        while (current)
        {
            folder_name_decoded.emplace_front(current->text(0).toStdString());
            current = current->parent();
        }
        std::string folder_name_s;
        for (auto s : folder_name_decoded)
        {
            folder_name_s += "\\" + s;
        }
        folder_name_decoded.clear();
        bool ok = false;
        std::string update_message;
        QString text = QInputDialog::getText(this, "Создание новой папки", "Название папки:", QLineEdit::Normal,
                                             QString::fromStdString(folder_name_s), &ok);
        if (ok && !text.isEmpty())
        {
            std::list<std::string> new_folder_name;
            std::string new_folder_name_s;
            for (auto s : text.split('\\'))
            {
                if (!s.isEmpty())
                {
                    new_folder_name.emplace_back(utf7encode(s).toStdString());
                    new_folder_name_s += "\\" + s.toStdString();
                }
            }
            try
            {
                std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
                mailio::imaps conn(imap_settings.get_server_address(),
                                   static_cast<unsigned>(imap_settings.get_server_port()), timeout);
                conn.authenticate(imap_settings.get_login(), imap_settings.get_password(),
                                  imap_settings.get_auth_method());
                ok = conn.create_folder(new_folder_name);
                if (!ok)
                {
                    throw mailio::imap_error("отказано в операции сервером");
                }
                std::chrono::duration<double> sec = std::chrono::system_clock::now() - start;
                update_message = "Папка " + new_folder_name_s + " добавлена (" + std::to_string(sec.count()) + " сек.)";
            }
            catch (const std::exception &exc)
            {
                update_message = "Папка " + new_folder_name_s + " не добавлена: " + std::string(exc.what());
            }
        }
        else
        {
            update_message = "Папка не добавлена: отмена операции";
        }
        std::string current_datetime_s = get_current_datetime_as_string();
        QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
        if (ok)
        {
            main_mutex.unlock();
            on_update_folders_and_messages_triggered();
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_rename_folder_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        if (ui->folders_tree->currentItem() == nullptr)
        {
            QMessageBox::warning(this, "Внимание", "Папка не выбрана");
            return;
        }
        std::list<std::string> old_folder_name, old_folder_name_decoded;
        QTreeWidgetItem *current = ui->folders_tree->currentItem();
        while (current)
        {
            old_folder_name_decoded.emplace_front(current->text(0).toStdString());
            old_folder_name.emplace_front(current->text(3).toStdString());
            current = current->parent();
        }
        std::string old_folder_name_s;
        for (auto s : old_folder_name_decoded)
        {
            old_folder_name_s += "\\" + s;
        }
        bool ok = false;
        std::string update_message;
        QString text =
            QInputDialog::getText(this, "Переименование/перемещение папки", "Новое название папки:", QLineEdit::Normal,
                                  QString::fromStdString(old_folder_name_s), &ok);
        if (ok && !text.isEmpty())
        {
            try
            {
                std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
                mailio::imaps conn(imap_settings.get_server_address(),
                                   static_cast<unsigned>(imap_settings.get_server_port()), timeout);
                conn.authenticate(imap_settings.get_login(), imap_settings.get_password(),
                                  imap_settings.get_auth_method());
                std::list<std::string> new_folder_name;
                std::string new_folder_name_s;
                for (auto s : text.split('\\'))
                {
                    if (!s.isEmpty())
                    {
                        new_folder_name.emplace_back(utf7encode(s).toStdString());
                        new_folder_name_s += "\\" + s.toStdString();
                    }
                }
                ok = conn.rename_folder(old_folder_name, new_folder_name);
                if (!ok)
                {
                    throw mailio::imap_error("отказано в операции сервером");
                }
                std::chrono::duration<double> sec = std::chrono::system_clock::now() - start;
                update_message = "Папка " + old_folder_name_s + " переименована -> " + new_folder_name_s + " (" +
                                 std::to_string(sec.count()) + " сек.)";
            }
            catch (const std::exception &exc)
            {
                update_message = "Папка " + old_folder_name_s + " не переименована: " + std::string(exc.what());
            }
        }
        else
        {
            update_message = "Папка " + old_folder_name_s + " не переименована: отмена операции";
        }
        std::string current_datetime_s = get_current_datetime_as_string();
        QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
        if (ok)
        {
            main_mutex.unlock();
            on_update_folders_and_messages_triggered();
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_delete_selected_folder_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        std::list<std::string> folder_name, folder_name_decoded;
        std::string folder_name_s;
        QTreeWidgetItem *current = ui->folders_tree->currentItem();
        while (current)
        {
            folder_name_decoded.emplace_front(current->text(0).toStdString());
            folder_name.emplace_front(current->text(3).toStdString());
            current = current->parent();
        }
        for (auto s : folder_name_decoded)
        {
            folder_name_s += "\\" + s;
        };
        std::string update_message;
        bool ok = (QMessageBox::question(
                       this, "Внимание",
                       QString::fromStdString("Вы уверены, что хотите удалить папку " + folder_name_s + "?"),
                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes);
        if (ok)
        {
            try
            {
                std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
                mailio::imaps conn(imap_settings.get_server_address(),
                                   static_cast<unsigned>(imap_settings.get_server_port()), timeout);
                conn.authenticate(imap_settings.get_login(), imap_settings.get_password(),
                                  imap_settings.get_auth_method());
                ok = conn.delete_folder(folder_name);
                if (!ok)
                {
                    throw mailio::imap_error("отказано в операции сервером");
                }
                std::chrono::duration<double> sec = std::chrono::system_clock::now() - start;
                update_message = "Папка " + folder_name_s + " удалена (" + std::to_string(sec.count()) + " сек.)";
            }
            catch (const std::exception &exc)
            {
                update_message = "Папка " + folder_name_s + " не удалена: " + std::string(exc.what());
            }
        }
        else
        {
            update_message = "Папка " + folder_name_s + " не удалена: отмена операции";
        }
        std::string current_datetime_s = get_current_datetime_as_string();
        QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
        if (ok)
        {
            main_mutex.unlock();
            on_update_folders_and_messages_triggered();
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_send_message_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        mailio::smtps conn(smtp_settings.get_server_address(), smtp_settings.get_server_port());
        conn.authenticate(smtp_settings.get_login(), smtp_settings.get_password(), smtp_settings.get_auth_method());
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
        return;
    }
    SendMessageDialog new_send_message_dialog;
    try
    {
        while (new_send_message_dialog.exec() == QDialog::Accepted)
        {
            try
            {
                mailio::message message = new_send_message_dialog.get_message();
                if (message.subject().empty())
                {
                    QMessageBox::warning(this, "Ошибка", "Тема сообщения пуста");
                    continue;
                }
                if (message.recipients().empty() && message.cc_recipients().empty() && message.bcc_recipients().empty())
                {
                    QMessageBox::warning(this, "Ошибка", "Получатели отсутствуют");
                    continue;
                }
                message.from(mailio::mail_address(smtp_settings.get_name(), smtp_settings.get_login()));
                mailio::smtps conn(smtp_settings.get_server_address(), smtp_settings.get_server_port());
                conn.authenticate(smtp_settings.get_login(), smtp_settings.get_password(),
                                  smtp_settings.get_auth_method());
                std::string status = conn.submit(message);
                std::string update_message = "Сообщение отправлено, обновите "
                                             "список папок с сообщениями";
                std::string current_datetime_s = get_current_datetime_as_string();
                QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
                ui->status_bar->showMessage(log_message);
                ui->logs_text_edit->appendPlainText(log_message);
                QMessageBox::information(this, "Информация",
                                         "Сообщение отправлено\nОбновите список папок "
                                         "с сообщениями\n\nОтвет сервера:\n" +
                                             QString::fromStdString(status));
                break;
            }
            catch (const std::exception &exc)
            {
                QMessageBox::warning(this, "Ошибка", exc.what());
            }
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_download_attachment_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        QTableWidgetItem *current_item = ui->messages_table->currentItem();
        if (current_item == nullptr)
        {
            QMessageBox::warning(this, "Внимание", "Сообщение не выбрано");
            return;
        }
        std::string current_message_id =
            ui->messages_table->item(ui->messages_table->currentRow(), 7)->text().toStdString();
        if (messages.find(current_message_id) == messages.end())
        {
            QMessageBox::warning(this, "Внимание", "Сообщение не найдено");
            return;
        }
        mailio::message current_message = messages[current_message_id];
        size_t n_attachments = current_message.attachments_size();
        if (n_attachments <= 0)
        {
            QMessageBox::warning(this, "Внимание", "Вложения в сообщении не найдены");
            return;
        }
        QStringList attachment_names, attachment_names_and_sizes;
        std::vector<std::shared_ptr<std::stringstream>> attachments_data_streams;
        constexpr unsigned long long kb = 1 << 10, mb = kb << 10, gb = mb << 10;
        for (size_t i = 1; i <= n_attachments; ++i)
        {
            std::string attachment_name, attachment_size;
            std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>();
            std::istream &is = *ss;
            std::ostream &os = *ss;
            current_message.attachment(i, os, attachment_name);
            is.seekg(0, std::ios::end);
            unsigned long long size = is.tellg();
            is.seekg(0, std::ios::beg);
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
            attachment_names.emplace_back(QString::fromStdString(attachment_name));
            attachment_names_and_sizes.emplace_back(
                QString::fromStdString(attachment_name + " (" + attachment_size + ")"));
            attachments_data_streams.emplace_back(ss);
        }
        bool ok = false;
        std::string update_message;
        QString filename_with_size = QInputDialog::getItem(this, "Внимание", "Какое вложение вы хотите скачать?",
                                                           attachment_names_and_sizes, 0, false, &ok);
        if (ok)
        {
            size_t p = 0;
            if ((p = attachment_names_and_sizes.indexOf(filename_with_size)) < 0)
            {
                QMessageBox::warning(this, "Внимание", "Вложение не найдено");
                return;
            }
            QString new_filename = QInputDialog::getText(this, "Внимание", "Введите путь и/или имя файла для записи",
                                                         QLineEdit::Normal, attachment_names[p], &ok);
            if (ok)
            {
                try
                {
                    std::ofstream file_out(new_filename.toStdWString().c_str(), std::ios::out | std::ios::binary);
                    file_out << ((std::istream &)*attachments_data_streams[p]).rdbuf();
                    file_out.close();
                    update_message = "Файл " + new_filename.toStdString() + " записан";
                }
                catch (const std::exception &exc)
                {
                    update_message = "Ошибка записи файла на диск: " + std::string(exc.what());
                }
            }
            else
            {
                update_message = "Сохранение файла отменено";
            }
        }
        else
        {
            update_message = "Сохранение файла отменено";
        }
        std::string current_datetime_s = get_current_datetime_as_string();
        QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
        ui->status_bar->showMessage(log_message);
        ui->logs_text_edit->appendPlainText(log_message);
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_copy_selected_message_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        QItemSelectionModel *select = ui->messages_table->selectionModel();
        if (!select->hasSelection())
        {
            QMessageBox::warning(this, "Внимание", "Сообщения не выбраны");
            return;
        }
        QList<QString> folders;
        for (auto folder : mailboxes_to_messages_ids)
        {
            QString folder_to_emplace;
            for (auto s : folder.first)
            {
                folder_to_emplace += "\\" + utf7decode(QString::fromStdString(s));
            }
            folders.emplace_back(folder_to_emplace);
        }
        bool ok = false;
        QString text =
            QInputDialog::getItem(this, "Копирование сообщений в папку", "Название папки:", folders, 0, false, &ok);
        if (!ok || text.isEmpty())
        {
            return;
        }
        std::list<std::string> destination_folder_name;
        for (auto s : text.split('\\'))
        {
            if (!s.isEmpty())
            {
                destination_folder_name.emplace_back(utf7encode(s).toStdString());
            }
        }
        try
        {
            std::string update_message;
            QModelIndexList selection = select->selectedIndexes();
            if (selection.count() == 0)
            {
                QMessageBox::warning(this, "Внимание", "Сообщения не выбраны");
                return;
            }
            std::unordered_set<int> rows;
            for (qsizetype i = 0; i < selection.count(); i++)
            {
                rows.insert(selection.at(i).row());
            }
            std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
            mailio::imaps conn(imap_settings.get_server_address(),
                               static_cast<unsigned>(imap_settings.get_server_port()), timeout);
            conn.authenticate(imap_settings.get_login(), imap_settings.get_password(), imap_settings.get_auth_method());
            for (auto row : rows)
            {
                QTableWidgetItem *current_uid = ui->messages_table->item(row, 6);
                if (current_uid == nullptr || current_uid->text().isEmpty())
                {
                    QMessageBox::warning(this, "Внимание", "UID одного сообщения не найден");
                    continue;
                }
                ok = false;
                unsigned long uid = current_uid->text().toULong(&ok);
                if (!ok)
                {
                    QMessageBox::warning(this, "Внимание", "Неверный UID у одного сообщения");
                    continue;
                }
                QTableWidgetItem *current_message_id = ui->messages_table->item(row, 7);
                if (current_message_id == nullptr || current_message_id->text().isEmpty())
                {
                    QMessageBox::warning(this, "Внимание", "Message ID одного сообщения не найден");
                    continue;
                }
                std::string current_message_id_s = current_message_id->text().toStdString();
                std::string datetime_s = ui->messages_table->item(row, 2)->text().toStdString();
                try
                {
                    conn.append(destination_folder_name, this->messages[current_message_id_s]);
                    std::chrono::duration<double> sec = std::chrono::system_clock::now() - start;
                    update_message = "Сообщение #" + std::to_string(uid) + " (" + datetime_s + ") скопировано (" +
                                     std::to_string(sec.count()) + " сек.)";
                }
                catch (const std::exception &exc)
                {
                    update_message = "Сообщение #" + std::to_string(uid) + " (" + datetime_s +
                                     ") не скопировано: " + std::string(exc.what());
                }
                std::string current_datetime_s = get_current_datetime_as_string();
                QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
                ui->status_bar->showMessage(log_message);
                ui->logs_text_edit->appendPlainText(log_message);
                ui->current_message->setDocument(&this->empty_text_document);
            }
            main_mutex.unlock();
            on_update_folders_and_messages_triggered();
        }
        catch (const std::exception &exc)
        {
            QMessageBox::critical(this, "Ошибка", exc.what());
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_delete_selected_message_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        QTreeWidgetItem *current_folder = ui->folders_tree->currentItem();
        std::list<std::string> folder_name;
        if (current_folder == nullptr)
        {
            if (this->current_item.empty())
            {
                QMessageBox::warning(this, "Внимание", "Папка не выбрана");
                return;
            }
            folder_name = this->current_item;
        }
        else
        {
            QTreeWidgetItem *current = current_folder;
            while (current)
            {
                folder_name.emplace_front(current->text(3).toStdString());
                current = current->parent();
            }
            this->current_item = folder_name;
        }
        QItemSelectionModel *select = ui->messages_table->selectionModel();
        if (!select->hasSelection())
        {
            QMessageBox::warning(this, "Внимание", "Сообщения не выбраны");
            return;
        }
        bool ok = (QMessageBox::question(this, "Внимание", "Вы уверены, что хотите удалить выбранные сообщения?",
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes);
        if (!ok)
        {
            return;
        }
        try
        {
            std::string update_message;
            QModelIndexList selection = select->selectedIndexes();
            if (selection.count() == 0)
            {
                QMessageBox::warning(this, "Внимание", "Сообщения не выбраны");
                return;
            }
            std::unordered_set<int> rows;
            for (qsizetype i = 0; i < selection.count(); i++)
            {
                rows.insert(selection.at(i).row());
            }
            std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
            mailio::imaps conn(imap_settings.get_server_address(),
                               static_cast<unsigned>(imap_settings.get_server_port()), timeout);
            conn.authenticate(imap_settings.get_login(), imap_settings.get_password(), imap_settings.get_auth_method());
            for (auto row : rows)
            {
                QTableWidgetItem *current_uid = ui->messages_table->item(row, 6);
                if (current_uid == nullptr || current_uid->text().isEmpty())
                {
                    QMessageBox::warning(this, "Внимание", "UID одного сообщения не найден");
                    continue;
                }
                ok = false;
                unsigned long uid = current_uid->text().toULong(&ok);
                if (!ok)
                {
                    QMessageBox::warning(this, "Внимание", "Неверный UID у одного сообщения");
                    continue;
                }
                std::string datetime_s = ui->messages_table->item(row, 2)->text().toStdString();
                try
                {
                    conn.remove(folder_name, uid, true);
                    std::chrono::duration<double> sec = std::chrono::system_clock::now() - start;
                    update_message = "Сообщение #" + std::to_string(uid) + " (" + datetime_s + ") удалено (" +
                                     std::to_string(sec.count()) + " сек.)";
                }
                catch (const std::exception &exc)
                {
                    update_message = "Сообщение #" + std::to_string(uid) + " (" + datetime_s +
                                     ") не удалено: " + std::string(exc.what());
                }
                std::string current_datetime_s = get_current_datetime_as_string();
                QString log_message = QString::fromStdString(current_datetime_s + ". " + update_message);
                ui->status_bar->showMessage(log_message);
                ui->logs_text_edit->appendPlainText(log_message);
                ui->current_message->setDocument(&this->empty_text_document);
            }
            main_mutex.unlock();
            on_update_folders_and_messages_triggered();
        }
        catch (const std::exception &exc)
        {
            QMessageBox::critical(this, "Ошибка", exc.what());
        }
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_clear_all_and_disable_autoupdate_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    clear_all();
}

void MainWindow::on_show_message_statistics_triggered()
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        int n_folders = ui->folders_tree->topLevelItemCount(), n_messages = 0, n_unseen_messages = 0, n_attachments = 0;
        for (int row = 0; row < ui->folders_tree->topLevelItemCount(); ++row)
        {
            n_folders += get_number_of_folders_recursive(ui->folders_tree->topLevelItem(row));
            n_messages += ui->folders_tree->topLevelItem(row)->text(2).toInt();
            n_unseen_messages += ui->folders_tree->topLevelItem(row)->text(1).toInt();
        }
        unsigned long long size = 0;
        constexpr unsigned long long kb = 1 << 10, mb = kb << 10, gb = mb << 10;
        for (auto message_size : this->messages_sizes)
        {
            size += message_size.second;
        }
        std::string size_s;
        if (size >= gb)
        {
            size = size * 100 / gb;
            size_s = std::to_string(size / 100) + "." + std::to_string(size % 100) + " ГиБ";
        }
        else if (size >= mb)
        {
            size = size * 100 / mb;
            size_s = std::to_string(size / 100) + "." + std::to_string(size % 100) + " МиБ";
        }
        else if (size >= kb)
        {
            size = size * 100 / kb;
            size_s = std::to_string(size / 100) + "." + std::to_string(size % 100) + " КиБ";
        }
        else
        {
            size_s = std::to_string(size) + " Б";
        }
        QString s = QString::fromStdString("Общее число папок, включая вложенные: " + std::to_string(n_folders) +
                                           "\nОбщее число писем во всех папках: " + std::to_string(n_messages) +
                                           "\nОбщее число непрочитанных писем: " + std::to_string(n_unseen_messages) +
                                           "\nОбщий размер закэшированных сообщений с вложениями: " + size_s);
        QMessageBox::information(this, "Статистика", s);
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

int MainWindow::get_number_of_folders_recursive(QTreeWidgetItem *twi)
{
    int n = twi->childCount();
    if (twi)
    {
        for (int c = 0, child_count = twi->childCount(); c < child_count; ++c)
        {
            n += get_number_of_folders_recursive(twi->child(c));
        }
    }
    return n;
}

void MainWindow::on_show_about_window_triggered()
{
    try
    {
        QMessageBox::about(this, "О программе",
                           "Курсовая работа по дисциплине ММиВА (СПбГУТ)\n"
                           "Тема: Разработка клиентского ПО для получения и "
                           "отправки почтовых сообщений\n"
                           "Выполнил студент группы ИКПИ-84\n"
                           "Коваленко Леонид\n"
                           "Санкт-Петербург\n"
                           "2022 год");
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_folders_tree_itemClicked(QTreeWidgetItem *,
                                             int) // not use arguments because concurrent programming!
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    folder_chosen_ui();
}

void MainWindow::on_messages_table_itemClicked(QTableWidgetItem *)
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    try
    {
        QTableWidgetItem *current_message_id = ui->messages_table->item(ui->messages_table->currentRow(), 7);
        if (current_message_id == nullptr || current_message_id->text().isEmpty())
        {
            ui->current_message->setDocument(&this->empty_text_document);
            ui->status_bar->showMessage("Message ID выбранного сообщения не найден");
            return;
        }
        std::string current_message_id_s = current_message_id->text().toStdString();
        if (this->messages_ids_to_text_documents.find(current_message_id_s) ==
            this->messages_ids_to_text_documents.end())
        {
            ui->current_message->setDocument(&this->empty_text_document);
            ui->status_bar->showMessage("Тело выбранного сообщения не найдено");
            return;
        }
        ui->current_message->setDocument(&this->messages_ids_to_text_documents[current_message_id_s]);
        ui->status_bar->showMessage("Сообщение открыто");
    }
    catch (const std::exception &exc)
    {
        QMessageBox::critical(this, "Ошибка", exc.what());
    }
}

void MainWindow::on_search_query_textChanged(const QString &query)
{
    ui->messages_table->setCurrentCell(-1, -1);
    if (query == "")
    {
        return;
    }
    std::lock_guard<std::mutex> main_lock(main_mutex);
    auto find_items = ui->messages_table->findItems(query, Qt::MatchContains);
    std::unordered_set<int> rows;
    for (qsizetype i = 0, n = find_items.size(); i < n; ++i)
    {
        auto item = find_items.at(i);
        item->setSelected(true);
        rows.insert(item->row());
    }
    ui->status_bar->showMessage("Найдено " + QString::number(rows.size()) + " сообщений");
}

void MainWindow::on_search_query_returnPressed()
{
    MainWindow::on_search_query_textChanged(ui->search_query->text());
}

std::string MainWindow::get_current_datetime_as_string()
{
    char buffer[256];
    std::time_t t = std::time(nullptr);
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&t)))
    {
        return std::string(buffer);
    }
    return "[undefined]";
}

std::string MainWindow::local_date_time_as_string(const boost::local_time::local_date_time &ldt)
{
    char buffer[256];
    auto dt_tm = boost::local_time::to_tm(ldt);
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &dt_tm))
    {
        return std::string(buffer);
    }
    return "[undefined]";
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    std::lock_guard<std::mutex> main_lock(main_mutex);
    if (QMessageBox::question(this, "Внимание", "Вы уверены, что хотите выйти?", QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::No) == QMessageBox::Yes)
    {
        clear_all();
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::clear_all()
{
    try
    {
        imap_settings.set_autoupdate(false);
        ui->folders_tree->clear();
        ui->messages_table->clearContents();
        ui->messages_table->setRowCount(0);
        ui->current_message->setDocument(&this->empty_text_document);
        ui->logs_text_edit->clear();
        ui->search_query->clear();
        ui->status_bar->clearMessage();
        update_status = true;
        update_status_message.clear();
        folders.clear();
        current_item.clear();
        messages_sizes.clear();
        messages.clear();
        messages_ids_to_uids.clear();
        messages_ids_to_text_documents.clear();
        uids_to_messages_ids.clear();
        mailboxes_to_messages_ids.clear();
    }
    catch (...)
    {
    }
}
