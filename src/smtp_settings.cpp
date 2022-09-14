#include "smtp_settings.h"

SmtpSettings::SmtpSettings()
{
}

// setters
void SmtpSettings::set_server_address(const std::string &server_address)
{
    this->server_address = server_address;
}
void SmtpSettings::set_server_port(int server_port)
{
    this->server_port = server_port;
}
void SmtpSettings::set_login(const std::string &login)
{
    this->login = login;
}
void SmtpSettings::set_password(std::string password)
{
    for (size_t i = 0, n = password.length(); i < n; ++i)
    {
        password[i] ^= 0x10100110 ^ i;
    }
    this->password = password;
}
void SmtpSettings::set_name(const std::string &name)
{
    this->name = name;
}
void SmtpSettings::set_auth_method(const mailio::smtps::auth_method_t &auth_method)
{
    this->auth_method = auth_method;
}

// getters
std::string SmtpSettings::get_server_address() const
{
    return this->server_address;
}
int SmtpSettings::get_server_port() const
{
    return this->server_port;
}
std::string SmtpSettings::get_login() const
{
    return this->login;
}
std::string SmtpSettings::get_password() const
{
    std::string decoded_password(password);
    for (size_t i = 0, n = decoded_password.length(); i < n; ++i)
    {
        decoded_password[i] ^= 0x10100110 ^ i;
    }
    return decoded_password;
}
std::string SmtpSettings::get_name() const
{
    return this->name;
}
mailio::smtps::auth_method_t SmtpSettings::get_auth_method() const
{
    return this->auth_method;
}
