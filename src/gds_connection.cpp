#include "gds_connection.hpp"
#include "gds_clients.hpp"

namespace gds_lib {

namespace connection {

    GDSInterface::~GDSInterface() {}

    std::shared_ptr<GDSInterface>
    GDSBuilder::build() const
    {
        if(tls.first.length() && tls.second.length())
        {
            return std::make_shared<gds_lib::client::SecureGDSClient>(uri, callbacks, username, timeout, tls.first, tls.second);
        }
        else
        {
            return std::make_shared<gds_lib::client::InsecureGDSClient>(uri, callbacks, username, password, timeout);
        }
    }
    /*
    std::shared_ptr<GDSInterface>
    GDSInterface::create(const std::string& url,  std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username)
    {
        return std::make_shared<gds_lib::client::InsecureGDSClient>(url, callbacks, username);
    }

    std::shared_ptr<GDSInterface>
    GDSInterface::create(const std::string& url,  std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks,  const std::string& username, const std::string& password)
    {
        return std::make_shared<gds_lib::client::InsecureGDSClient>(url, callbacks, username, password);
    }

    std::shared_ptr<GDSInterface>
    GDSInterface::create(const std::string& url, std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, const std::string& cert, const std::string& cert_pw)
    {
        return std::make_shared<gds_lib::client::SecureGDSClient>(url, callbacks, username, cert, cert_pw);
    }
    */


} // namespace connection
} // namespace gds_lib