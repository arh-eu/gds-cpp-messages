#include "io_service.hpp"

#include <chrono>

IoService::IoService()
    : mIoService(std::make_shared<asio::io_service>()), mClose(false),
      mThread(std::bind(&IoService::IoServiceThread, this)) {}

IoService::~IoService() { stop(); }

void IoService::stop() {
  mClose.store(true);
  if (mThread.joinable()) {
    mThread.join();
  }
}

const gds_lib::connection::io_service_sptr IoService::getIoService() const {
  return mIoService;
}

void IoService::IoServiceThread() {
  while (!mClose.load() && mIoService) {
    if (mIoService->stopped()) {
      mIoService->restart();
    }
    mIoService->run();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
