#ifndef SMTPSETTINGS_H
#define SMTPSETTINGS_H

#include <mailio/smtp.hpp>

class SmtpSettings
{
  public:
    SmtpSettings();
    // setters
    void set_server_address(const std::string &server_address);
    void set_server_port(int server_port);
    void set_login(const std::string &login);
    void set_password(std::string password);
    void set_name(const std::string &name);
    void set_auth_method(const mailio::smtps::auth_method_t &auth_method);
    // getters
    std::string get_server_address() const;
    int get_server_port() const;
    std::string get_login() const;
    std::string get_password() const;
    std::string get_name() const;
    mailio::smtps::auth_method_t get_auth_method() const;

  private:
    std::string server_address, login, password, name;
    int server_port;
    mailio::smtps::auth_method_t auth_method;
};

#endif // SMTPSETTINGS_H
