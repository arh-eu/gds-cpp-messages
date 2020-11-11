#ifndef GDS_CLIENTS_HPP
#define GDS_CLIENTS_HPP

#include "gds_certs.hpp"
#include "gds_connection.hpp"
#include "gds_uuid.hpp"

#include "countdownlatch.hpp"

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
        std::shared_ptr<gds_lib::connection::GDSMessageListener> mCallbacks;
        connection_sptr mConnection;

        thread_utils::CountDownLatch mCountdownlatch;

        void m_on_open(connection_sptr connection);
        void m_on_message(connection_sptr /*connection*/,
            std::shared_ptr<typename ws_client_type::InMessage> message);
        void m_on_close(connection_sptr /*connection*/, int status,
            const std::string& reason);
        void m_on_error(connection_sptr /*connection*/,
            const SimpleWeb::error_code& error_code);

    public:
        //NO / PASSWORD AUTH
        BaseGDSClient(const std::string& url, std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, const std::string& password, const uint64_t timeout);
        //TLS AUTH
        BaseGDSClient(const std::string& url, std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, const uint64_t timeout, const std::string& cert, const std::string& cert_pw);

        BaseGDSClient(const BaseGDSClient<ws_client_type>&) = delete;
        BaseGDSClient(const BaseGDSClient<ws_client_type>&&) = delete;

        BaseGDSClient& operator=(const BaseGDSClient<ws_client_type>&) = delete;
        BaseGDSClient& operator=(const BaseGDSClient<ws_client_type>&&) = delete;

        ~BaseGDSClient();

        void send(const gds_lib::gds_types::GdsMessage& msg) override;
        gds_lib::connection::State get_state() override;
        void start() override;
        void close() override;

    private:
        void login();
        void init();
        bool m_closed;
        bool m_started;
        bool m_logged_in;
        std::pair <std::string,std::string> tls_files;
        std::string m_username;
        std::string m_password;
        uint64_t m_timeout;

        std::atomic<gds_lib::connection::State> m_state;
    };

    using InsecureGDSClient = BaseGDSClient<SimpleWeb::SocketClient<SimpleWeb::WS> >;
    using SecureGDSClient = BaseGDSClient<SimpleWeb::SocketClient<SimpleWeb::WSS> >;

    //impl.

    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url,
     std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, const std::string& password, const uint64_t timeout)
    : mWebSocket(std::make_shared<ws_client_type>(url)), mCallbacks(callbacks), mCountdownlatch(1), m_username(username), m_password(password), m_timeout(timeout)
    {
        init();
    }

    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::BaseGDSClient(const std::string& url,  std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, 
        const uint64_t timeout, const std::string& cert_path, const std::string& cert_pw)
    :mCallbacks(callbacks), mCountdownlatch(1), m_username(username), m_timeout(timeout)
    {
        tls_files = parse_cert(cert_path, cert_pw);
        mWebSocket = std::make_shared<ws_client_type>(url, false, tls_files.first, tls_files.second);
        init();
    }


    template <typename ws_client_type>
    BaseGDSClient<ws_client_type>::~BaseGDSClient()
    {
        close();
    }

    template <typename ws_client_type>
    gds_lib::connection::State BaseGDSClient<ws_client_type>::get_state()
    {
        return m_state.load();
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::init()
    {
        if(mCallbacks.get() == nullptr) {
            throw std::logic_error("The listener cannot be null!");
        }

        mWebSocket->on_open = std::bind(&BaseGDSClient<ws_client_type>::m_on_open, this, std::placeholders::_1);
        mWebSocket->on_message = std::bind(&BaseGDSClient<ws_client_type>::m_on_message, this, std::placeholders::_1, std::placeholders::_2);
        mWebSocket->on_close = std::bind(&BaseGDSClient<ws_client_type>::m_on_close, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        mWebSocket->on_error = std::bind(&BaseGDSClient<ws_client_type>::m_on_error, this, std::placeholders::_1, std::placeholders::_2);

        m_closed = false;
        m_started = false;

        m_state.store(gds_lib::connection::State::NOT_CONNECTED);
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::start()
    {

        gds_lib::connection::State old_state = gds_lib::connection::State::NOT_CONNECTED;
        if(!m_state.compare_exchange_strong(old_state, gds_lib::connection::State::INITIALIZING)) {
            throw gds_lib::connection::state_error(gds_lib::connection::State::NOT_CONNECTED, old_state, "start()");
        }

        
        std::thread m_wsThread = std::thread([this]() {

            gds_lib::connection::State old_state = gds_lib::connection::State::INITIALIZING;
            if(!this->m_state.compare_exchange_strong(old_state, gds_lib::connection::State::CONNECTING)) {
                throw gds_lib::connection::state_error(gds_lib::connection::State::INITIALIZING, old_state, "start()");
            }

            this->mWebSocket->start();
            this->mWebSocket->io_service->run();

            if(!this->mCountdownlatch.await(m_timeout)) {
                gds_lib::connection::State old_state = this->get_state();
                if(old_state != gds_lib::connection::State::FAILED && old_state != gds_lib::connection::State::DISCONNECTED){
                    this->m_state.store(gds_lib::connection::State::FAILED);
                    if (this->mConnection) {
                        this->mConnection->send_close(1000);
                        this->mConnection.reset();
                    }
                    this->mCallbacks->on_connection_failure(gds_lib::connection::connection_error("The GDS did not respond within the specified timeout!"), {});
                }
            }

            if(this->tls_files.first.size())
            {
                std::remove(this->tls_files.first.c_str());
            }

            if(this->tls_files.second.size())
            {
                std::remove(this->tls_files.second.c_str());
            }
        });
        m_wsThread.detach();
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_message(connection_sptr /*connection*/,
        std::shared_ptr<typename ws_client_type::InMessage> in_msg)
    {
        std::string message = in_msg->string();
        msgpack::object_handle oh = msgpack::unpack(message.data(), message.size());
        msgpack::object replyMsg = oh.get();

        //gds_lib::gds_types::GdsMessage msg;
        gds_lib::gds_types::gds_message_t msg = std::make_shared<gds_types::GdsMessage>();
        try {
            msg->unpack(replyMsg);
            switch (msg->dataType) {
                case gds_types::GdsMsgType::LOGIN_REPLY: // Type 1
                {
                    mCountdownlatch.countdown();
                    gds_lib::connection::State old_state = get_state();
                    if(    old_state != gds_lib::connection::State::LOGGING_IN 
                        && old_state != gds_lib::connection::State::FAILED
                        && old_state != gds_lib::connection::State::DISCONNECTED){
                        throw gds_lib::connection::state_error(gds_lib::connection::State::LOGGING_IN, old_state, "on_message()");
                    }

                    std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage> body =
                    std::dynamic_pointer_cast<gds_types::GdsLoginReplyMessage>(msg->messageBody);
                    if(body->ackStatus == 200 ){
                        m_state.store(gds_lib::connection::State::LOGGED_IN);
                        mCallbacks->on_connection_success(msg, body);
                    }
                    else {
                        m_state.store(gds_lib::connection::State::FAILED);
                        close();
                        mCallbacks->on_connection_failure({}, std::make_pair(msg, body));
                    }

                } break;
                case gds_types::GdsMsgType::EVENT_REPLY: // Type 3
                {
                    std::shared_ptr<gds_lib::gds_types::GdsEventReplyMessage> body = 
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsEventReplyMessage>(msg->messageBody);
                    mCallbacks->on_event_ack3(msg, body);
                } break;
              case gds_types::GdsMsgType::ATTACHMENT_REQUEST: // Type 4
                {
                    std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestMessage> body = 
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsAttachmentRequestMessage>(msg->messageBody);
                    mCallbacks->on_attachment_request4(msg, body);
                } break;
                case gds_types::GdsMsgType::ATTACHMENT_REQUEST_REPLY: // Type 5
                {
                    std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage> body = 
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsAttachmentRequestReplyMessage>(msg->messageBody);
                    mCallbacks->on_attachment_request_ack5(msg, body);
                } break;
                case gds_types::GdsMsgType::ATTACHMENT: // Type 6
                {
                    std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage> body =
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsAttachmentResponseMessage>(msg->messageBody);
                    mCallbacks->on_attachment_response6(msg, body);
                } break;
                case gds_types::GdsMsgType::ATTACHMENT_REPLY: // Type 7
                {
                    std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseResultMessage> body =
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsAttachmentResponseResultMessage>(msg->messageBody);
                    mCallbacks->on_attachment_response_ack7(msg, body);
                } break;
                case gds_types::GdsMsgType::EVENT_DOCUMENT: // Type 8
                {
                    std::shared_ptr<gds_lib::gds_types::GdsEventDocumentMessage> body =
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsEventDocumentMessage>(msg->messageBody);
                    mCallbacks->on_event_document8(msg, body);
                } break;
                case gds_types::GdsMsgType::EVENT_DOCUMENT_REPLY: // Type 9
                {
                    std::shared_ptr<gds_lib::gds_types::GdsEventDocumentReplyMessage> body =
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsEventDocumentReplyMessage>(msg->messageBody);
                    mCallbacks->on_event_document_ack9(msg, body);
                } break;
                case gds_types::GdsMsgType::QUERY_REPLY: // Type 11
                {
                    std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage> body =
                    std::dynamic_pointer_cast<gds_lib::gds_types::GdsQueryReplyMessage>(msg->messageBody);
                    mCallbacks->on_query_request_ack11(msg, body);
                } break;
                default:

                    break;
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

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_open(connection_sptr connection)
    {
        gds_lib::connection::State old_state = gds_lib::connection::State::CONNECTING;
        if(!m_state.compare_exchange_strong(old_state, gds_lib::connection::State::CONNECTED)) {
            throw gds_lib::connection::state_error(gds_lib::connection::State::CONNECTING, old_state, "on_open(1)");
        }
        mConnection = connection;

        old_state = gds_lib::connection::State::CONNECTED;
        if(!m_state.compare_exchange_strong(old_state, gds_lib::connection::State::LOGGING_IN)) {
            throw gds_lib::connection::state_error(gds_lib::connection::State::CONNECTED, old_state, "on_open(2)");
        }
        login();
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_close(connection_sptr /*connection*/,
        int code, const std::string& reason)
    {
        m_state.store(gds_lib::connection::State::DISCONNECTED);
        /*
        if (on_close) {
            on_close(code, reason);
        }*/
        mCallbacks->on_disconnect();
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::m_on_error(connection_sptr /*connection*/,
        const SimpleWeb::error_code& error_code)
    {
        mCountdownlatch.countdown();
        m_state.store(gds_lib::connection::State::FAILED);
        close();
        std::string msg = error_code.message();
        mCallbacks->on_connection_failure(gds_lib::connection::connection_error(msg), {});
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

        std::shared_ptr<gds_lib::gds_types::GdsLoginMessage> loginBody = std::make_shared<gds_lib::gds_types::GdsLoginMessage>();
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
        {
            msgpack::sbuffer buffer;
            msgpack::packer<msgpack::sbuffer> pk(&buffer);
            fullMessage.pack(pk);
            std::shared_ptr<typename ws_client_type::OutMessage> stream = std::make_shared<typename ws_client_type::OutMessage>();
            stream->write(buffer.data(), buffer.size());
            mConnection->send(stream, nullptr, 130);

        }
    }

    template <typename ws_client_type>
    void BaseGDSClient<ws_client_type>::send(const gds_lib::gds_types::GdsMessage& msg)
    {
        if(get_state() != gds_lib::connection::State::LOGGED_IN) {
            throw std::runtime_error("Cannot send message without a successful login!");
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
        if(get_state() != gds_lib::connection::State::FAILED) {
            m_state.store(gds_lib::connection::State::DISCONNECTED);
        }

        if (mConnection) {
            mConnection->send_close(1000);
            mConnection.reset();
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