#ifndef IMAPSETTINGS_H
#define IMAPSETTINGS_H

#include <mailio/imap.hpp>

class ImapSettings
{
  public:
    ImapSettings();
    // setters
    void set_server_address(const std::string &server_address);
    void set_server_port(int server_port);
    void set_login(const std::string &login);
    void set_password(std::string password);
    void set_auth_method(const mailio::imaps::auth_method_t &auth_method);
    void set_autoupdate(bool autoupdate);
    void set_autoupdate_interval(int interval);
    // getters
    std::string get_server_address() const;
    int get_server_port() const;
    std::string get_login() const;
    std::string get_password() const;
    mailio::imaps::auth_method_t get_auth_method() const;
    bool get_autoupdate() const;
    int get_autoupdate_interval() const;

  private:
    std::string server_address, login, password;
    mailio::imaps::auth_method_t auth_method;
    int server_port, autoupdate_interval;
    bool autoupdate_enable;
};

#endif // IMAPSETTINGS_H
