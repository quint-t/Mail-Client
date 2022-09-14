#include "imap_settings.h"

ImapSettings::ImapSettings()
{
}

// setters
void ImapSettings::set_server_address(const std::string &server_address)
{
    this->server_address = server_address;
}
void ImapSettings::set_server_port(int server_port)
{
    this->server_port = server_port;
}
void ImapSettings::set_login(const std::string &login)
{
    this->login = login;
}
void ImapSettings::set_password(std::string password)
{
    for (size_t i = 0, n = password.length(); i < n; ++i)
    {
        password[i] ^= 0x01011001 ^ i;
    }
    this->password = password;
}
void ImapSettings::set_auth_method(const mailio::imaps::auth_method_t &auth_method)
{
    this->auth_method = auth_method;
}
void ImapSettings::set_autoupdate(bool autoupdate)
{
    this->autoupdate_enable = autoupdate;
}
void ImapSettings::set_autoupdate_interval(int interval)
{
    this->autoupdate_interval = interval;
}

// getters
std::string ImapSettings::get_server_address() const
{
    return this->server_address;
}
int ImapSettings::get_server_port() const
{
    return this->server_port;
}
std::string ImapSettings::get_login() const
{
    return this->login;
}
std::string ImapSettings::get_password() const
{
    std::string decoded_password(password);
    for (size_t i = 0, n = decoded_password.length(); i < n; ++i)
    {
        decoded_password[i] ^= 0x01011001 ^ i;
    }
    return decoded_password;
}
mailio::imaps::auth_method_t ImapSettings::get_auth_method() const
{
    return this->auth_method;
}
bool ImapSettings::get_autoupdate() const
{
    return this->autoupdate_enable;
}
int ImapSettings::get_autoupdate_interval() const
{
    return this->autoupdate_interval;
}
