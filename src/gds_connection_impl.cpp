#include "gds_connection_impl.hpp"
#include <iostream>

namespace gds_lib {

namespace connection {

GDSConnection::GDSConnection(const std::string &host, const std::string &port,
                             const std::string &    path,
                             const GDSCallbacks &   GDSCallback,
                             const io_service_sptr &io_service)
    : GDSConnection(host + ':' + port + '/' + path, GDSCallback, io_service) {}

GDSConnection::GDSConnection(const std::string &    url,
                             const GDSCallbacks &   GDSCallback,
                             const io_service_sptr &io_service)
    : mGDSCallback(GDSCallback), mWebSocket(url, io_service),
      mConnection(nullptr) {

  mCloseRequested.store(false);
  mClosed.store(false);
  mWebSocket.on_message =
      std::bind(&GDSConnection::onMessage, this, std::placeholders::_1,
                std::placeholders::_2);
  mWebSocket.on_open =
      std::bind(&GDSConnection::onOpen, this, std::placeholders::_1);
  mWebSocket.on_close =
      std::bind(&GDSConnection::onClose, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3);
  mWebSocket.on_error = std::bind(&GDSConnection::onError, this,
                                  std::placeholders::_1, std::placeholders::_2);

  mInMessageThread = std::thread(&GDSConnection::inTreadFunc, this);
  mOutMessageThread = std::thread(&GDSConnection::outTreadFunc, this);

  mWebSocket.start();
}

GDSConnection::~GDSConnection() { close(); }

void GDSConnection::sendMessage(const gds_lib::gds_types::GdsMessage &msg) {
  try {
    msg.validate();
    mOutMessages.push(msg);
    mOutSemaphore.notify();
  } catch (const gds_types::invalid_message_error &e) {
    mGDSCallback.msgErrorCallback(e);
  }
}

void GDSConnection::close() {
  if (!mClosed.load()) {
    mClosed.store(true);
    mCloseRequested.store(true);
    mInSemaphore.notify();
    mOutSemaphore.notify();
    if (mInMessageThread.joinable()) {
      mInMessageThread.join();
    }
    if (mOutMessageThread.joinable()) {
      mOutMessageThread.join();
    }
    if (mConnection) {
      mConnection->send_close(1000);
      mConnection.reset();
    }
    std::cerr << "API closed.." << std::endl;
    mGDSCallback.wsClosedCallback({1000, ""});
  }
}

void GDSConnection::onOpen(ws_connection_sptr connection) {
  mConnection = connection;
  mOutSemaphore.notify();
  mGDSCallback.connReadyCallback();
}

void GDSConnection::onMessage(ws_connection_sptr /*connection*/,
                              std::shared_ptr<ws_client_t::InMessage> message) {
  mInMessages.push(message->string());
  mInSemaphore.notify();
}

void GDSConnection::onClose(ws_connection_sptr /*connection*/, int status,
                            const std::string &reason) {
  close();
}

void GDSConnection::onError(ws_connection_sptr /*connection*/,
                            const SimpleWeb::error_code &error_code) {

  mGDSCallback.wsErrorCallback({error_code.value(), error_code.message()});
}

void GDSConnection::inTreadFunc() {
  while (!mCloseRequested.load()) {
    mInSemaphore.wait();
    if (!mCloseRequested.load()) {
      while (!mInMessages.empty()) {
        std::string            message = mInMessages.pop();
        msgpack::object_handle oh =
            msgpack::unpack(message.data(), message.size());
        msgpack::object replyMsg = oh.get();

        gds_lib::gds_types::GdsMessage msg;
        try {
          msg.unpack(replyMsg);
          mGDSCallback.msgCallback(msg);
        } catch (gds_lib::gds_types::invalid_message_error &e) {
          mGDSCallback.msgErrorCallback(e);
        } catch (msgpack::type_error &e) {
          mGDSCallback.msgErrorCallback(
              gds_lib::gds_types::invalid_message_error(
                  gds_lib::gds_types::GdsMsgType::UNKNOWN, e.what()));
        }
      }
    } else {
      return;
    }
  }
}

void GDSConnection::outTreadFunc() {
  while (!mCloseRequested.load() && !mConnection) {
    mOutSemaphore.wait();
  }
  while (!mCloseRequested.load()) {
    mOutSemaphore.wait();
    if (!mCloseRequested.load()) {
      while (!mOutMessages.empty()) {
        gds_lib::gds_types::GdsMessage next = mOutMessages.pop();

        msgpack::sbuffer                  buffer;
        msgpack::packer<msgpack::sbuffer> pk(&buffer);

        // validate is called before it's put into the queue so it _should not_
        // throw anything here
        next.pack(pk);

        std::shared_ptr<ws_client_t::OutMessage> stream =
            std::make_shared<ws_client_t::OutMessage>();
        stream->write(buffer.data(), buffer.size());
        mConnection->send(stream, nullptr, 130);
      }
    } else {
      return;
    }
  }
}

} // namespace connection
} // namespace gds_lib