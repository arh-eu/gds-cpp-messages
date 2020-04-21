#ifndef IO_SERVICE_HPP
#define IO_SERVICE_HPP

#include "gds_connection.hpp"

#include "asio.hpp"

#include <thread>

class IoService {
private:
  gds_lib::connection::io_service_sptr mIoService;
  std::atomic<bool>                    mClose;
  std::thread                          mThread;

public:
  /**
   * Creates a new io_service, and a worker thread
   */
  IoService();
  ~IoService();

  /**
   * Stops the io service thread
   */
  void stop();

  /**
   * Gives back an io service for further use
   * @return
   */
  const gds_lib::connection::io_service_sptr getIoService() const;

private:
  /**
   * Restarts and runs the io worker's worker object
   */
  void IoServiceThread();
};

#endif // IO_SERVICE_HPP
