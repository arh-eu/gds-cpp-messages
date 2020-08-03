#include "gds_connection.hpp"
#include "gds_clients.hpp"

namespace gds_lib {

	namespace connection {

		GDSInterface::~GDSInterface() {}

		std::shared_ptr<GDSInterface>
		GDSInterface::create(const std::string &url) {
			return std::make_shared<gds_lib::client::InsecureGDSClient>(url);
		}

		std::shared_ptr<GDSInterface>
		GDSInterface::create(const std::string &url, const std::string& cert, const std::string& cert_pw) {
			return std::make_shared<gds_lib::client::SecureGDSClient>(url, cert, cert_pw);
		}

} // namespace connection
} // namespace gds_lib