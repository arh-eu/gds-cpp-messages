#ifndef GDS_CLIENTS_HPP
#define GDS_CLIENTS_HPP

#include "gds_connection.hpp"

#include <iostream>
#include <memory>
#include <thread>

#include <simple-websocket-server/client_ws.hpp>
#include <simple-websocket-server/client_wss.hpp>

namespace gds_lib {
namespace client {

    template <typename ws_client_type>
    class BaseGDSClient : public gds_lib::connection::GDSInterface {
    protected:
        using connection_sptr = std::shared_ptr<typename ws_client_type::Connection>;

        std::shared_ptr<ws_client_type> mWebSocket;
        connection_sptr mConnection;

        void m_on_open(connection_sptr connection);
        void m_on_message(connection_sptr /*connection*/,
            std::shared_ptr<typename ws_client_type::InMessage> message);
        void m_on_close(connection_sptr /*connection*/, int status,
            const std::string& reason);
        void m_on_error(connection_sptr /*connection*/,
            const SimpleWeb::error_code& error_code);

    public:
        BaseGDSClient(const std::string& url);
        BaseGDSClient(const std::string& url, const std::string& cert, const std::string& cert_pw);

        BaseGDSClient(const BaseGDSClient<ws_client_type>&) = delete;
        BaseGDSClient(const BaseGDSClient<ws_client_type>&&) = delete;

        BaseGDSClient& operator=(const BaseGDSClient<ws_client_type>&) = delete;
        BaseGDSClient& operator=(const BaseGDSClient<ws_client_type>&&) = delete;
        ~BaseGDSClient();

        void send(const gds_lib::gds_types::GdsMessage& msg) override;
        void start() override;
        void close() override;

    private:
        void init();
        std::thread m_wsThread;
        bool m_closed;
        bool m_started;
    };

    using InsecureGDSClient = BaseGDSClient<SimpleWeb::SocketClient<SimpleWeb::WS> >;
    using SecureGDSClient = BaseGDSClient<SimpleWeb::SocketClient<SimpleWeb::WSS> >;

    //impl.
    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url)
        : mWebSocket(new ws_client_type(url))
    {
        init();
    }

    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url, const std::string& cert, const std::string& key)
    {
        mWebSocket.reset(new ws_client_type(url, false, cert, key));
        init();
    }

    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::~BaseGDSClient()
    {
        close();
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::init()
    {
        mWebSocket->on_open = std::bind(&BaseGDSClient<ws_client_type>::m_on_open, this, std::placeholders::_1);
        mWebSocket->on_message = std::bind(&BaseGDSClient<ws_client_type>::m_on_message, this, std::placeholders::_1, std::placeholders::_2);
        mWebSocket->on_close = std::bind(&BaseGDSClient<ws_client_type>::m_on_close, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        mWebSocket->on_error = std::bind(&BaseGDSClient<ws_client_type>::m_on_error, this, std::placeholders::_1, std::placeholders::_2);

        m_closed = false;
        m_started = false;
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::start()
    {
        if (!m_started) {
            m_wsThread = std::thread([this]() {
                this->mWebSocket->start();
                this->mWebSocket->io_service->run();
            });
            m_started = true;
        }
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_message(connection_sptr /*connection*/,
        std::shared_ptr<typename ws_client_type::InMessage> in_msg)
    {
        if (on_message) {
            std::string message = in_msg->string();
            msgpack::object_handle oh = msgpack::unpack(message.data(), message.size());
            msgpack::object replyMsg = oh.get();

            gds_lib::gds_types::GdsMessage msg;
            try {
                msg.unpack(replyMsg);
                on_message(msg);
            }
            catch (gds_lib::gds_types::invalid_message_error& e) {
                std::cerr << "Invalid format on the incoming message!" << std::endl;
                std::cerr << e.what() << std::endl;
            }
            catch (msgpack::type_error& e) {
                std::cerr << "MessagePack type error on the incoming message.." << std::endl;
                std::cerr << e.what() << std::endl;
            }
        }
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_open(connection_sptr connection)
    {
        mConnection = connection;
        if (on_open) {
            on_open();
        }
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_close(connection_sptr /*connection*/,
        int code, const std::string& reason)
    {
        if (on_close) {
            on_close(code, reason);
        }
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_error(connection_sptr /*connection*/,
        const SimpleWeb::error_code& error_code)
    {
        if (on_error) {
            on_error(error_code.value(), error_code.message());
        }
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::send(const gds_lib::gds_types::GdsMessage& msg)
    {
        if (!m_started) {
            throw new std::runtime_error("Cannot send message without 'start()' call!");
        }
        msgpack::sbuffer buffer;
        msgpack::packer<msgpack::sbuffer> pk(&buffer);

        msg.pack(pk);

        std::shared_ptr<typename ws_client_type::OutMessage> stream = std::make_shared<typename ws_client_type::OutMessage>();
        stream->write(buffer.data(), buffer.size());
        mConnection->send(stream, nullptr, 130);
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::close()
    {
        if (m_started && !m_closed) {
            if (mConnection) {
                std::cerr << "Closing WebSocket connection.." << std::endl;
                mConnection->send_close(1000);
            }

            if (m_wsThread.joinable()) {
                m_wsThread.join();
            }
            m_closed = true;
        }
    }

    /*	
		template
		<typename ws_client_type>
		void BaseGDSClient<ws_client_type>::
		{

		}
*/
}
}

#endif // GDS_CLIENTS_HPP