#include "gds_connection.hpp"
#include "gds_connection_impl.hpp"

namespace gds_lib {

namespace connection {

GDSInterface::~GDSInterface() {}

std::shared_ptr<GDSInterface>
GDSInterface::create(const std::string &host, const std::string &port,
                     const std::string &path, const GDSCallbacks &GDSCallback,
                     const io_service_sptr &io_service)

{
  return std::make_shared<GDSConnection>(host, port, path, GDSCallback,
                                         io_service);
}

std::shared_ptr<GDSInterface>
GDSInterface::create(const std::string &url, const GDSCallbacks &GDSCallback,
                     const io_service_sptr &io_service) {
  return std::make_shared<GDSConnection>(url, GDSCallback, io_service);
}

} // namespace connection
} // namespace gds_lib