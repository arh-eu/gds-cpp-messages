#ifndef GDS_CLIENTS_HPP
#define GDS_CLIENTS_HPP

#include "gds_certs.hpp"
#include "gds_connection.hpp"
#include "gds_uuid.hpp"

#include <iostream>
#include <chrono>
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
        //NO AUTH
        BaseGDSClient(const std::string& url, const std::string& username);
        //PASSWORD AUTH
        BaseGDSClient(const std::string& url, const std::string& username, const std::string& password);
        //TLS AUTH
        BaseGDSClient(const std::string& url, const std::string& username, const std::string& cert, const std::string& cert_pw);

        BaseGDSClient(const BaseGDSClient<ws_client_type>&) = delete;
        BaseGDSClient(const BaseGDSClient<ws_client_type>&&) = delete;

        BaseGDSClient& operator=(const BaseGDSClient<ws_client_type>&) = delete;
        BaseGDSClient& operator=(const BaseGDSClient<ws_client_type>&&) = delete;
        ~BaseGDSClient();
        void send(const gds_lib::gds_types::GdsMessage& msg) override;
        void start() override;
        void close() override;

    private:
        void login();
        void init();
        std::thread m_wsThread;
        bool m_closed;
        bool m_started;
        bool m_logged_in;
        std::pair <std::string,std::string> tls_files;
        std::string m_username;
        std::string m_password;
    };

    using InsecureGDSClient = BaseGDSClient<SimpleWeb::SocketClient<SimpleWeb::WS> >;
    using SecureGDSClient = BaseGDSClient<SimpleWeb::SocketClient<SimpleWeb::WSS> >;

    //impl.
    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url, const std::string& username)
        : mWebSocket(new ws_client_type(url)), m_username(username)
    {
        init();
    }

    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url, const std::string& username, const std::string& password)
    : mWebSocket(new ws_client_type(url)), m_username(username), m_password(password) 
    {
        init();
    }

    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url, const std::string& username, 
        const std::string& cert_path, const std::string& cert_pw)
    :m_username(username)
    {
        tls_files = parse_cert(cert_path, cert_pw);
        mWebSocket.reset(new ws_client_type(url, false, tls_files.first, tls_files.second));
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
                if(msg.dataType == gds_lib::gds_types::GdsMsgType::LOGIN_REPLY) {
                    std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage> body
                    = std::dynamic_pointer_cast<gds_lib::gds_types::GdsLoginReplyMessage>(msg.messageBody);

                    if(on_login) {
                        on_login(body->ackStatus == 200, body);                        
                    }

                } else {
                    on_message(msg);
                }
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
        if(on_open) {
            on_open();
        }
        login();
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
    void BaseGDSClient<ws_client_type>::login()
    {
        gds_lib::gds_types::GdsMessage fullMessage;

        using namespace std::chrono;
        auto currentTime= duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        fullMessage.userName = m_username;
        fullMessage.messageId = uuid::generate_uuid_v4();
        fullMessage.createTime = currentTime;
        fullMessage.requestTime = currentTime;
        fullMessage.isFragmented = false;

        fullMessage.dataType = gds_lib::gds_types::GdsMsgType::LOGIN;

        std::shared_ptr<gds_lib::gds_types::GdsLoginMessage> loginBody(new gds_lib::gds_types::GdsLoginMessage());
        {
            loginBody->serve_on_the_same_connection = false;
            loginBody->protocol_version_number = (5 << 16 | 1);
            loginBody->fragmentation_supported = false;
            if(m_password.length()) {
                loginBody->reserved_fields = std::vector<std::string>{};
                loginBody->reserved_fields.value().emplace_back(m_password);
            }
        }
        fullMessage.messageBody = loginBody;
        send(fullMessage);
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

            if(tls_files.first.size())
            {
                std::remove(tls_files.first.c_str());
            }

            if(tls_files.second.size())
            {
                std::remove(tls_files.second.c_str());
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