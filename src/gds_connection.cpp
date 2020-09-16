#include "gds_connection.hpp"
#include "gds_clients.hpp"

namespace gds_lib {

namespace connection {

    GDSInterface::~GDSInterface() {}

    std::shared_ptr<GDSInterface>
    GDSInterface::create(const std::string& url, const std::string& username)
    {
        return std::make_shared<gds_lib::client::InsecureGDSClient>(url, username);
    }

    std::shared_ptr<GDSInterface>
    GDSInterface::create(const std::string& url, const std::string& username, const std::string& password)
    {
        return std::make_shared<gds_lib::client::InsecureGDSClient>(url, username, password);
    }

    std::shared_ptr<GDSInterface>
    GDSInterface::create(const std::string& url, const std::string& username, const std::string& cert, const std::string& cert_pw)
    {
        return std::make_shared<gds_lib::client::SecureGDSClient>(url, username, cert, cert_pw);
    }

} // namespace connection
} // namespace gds_lib