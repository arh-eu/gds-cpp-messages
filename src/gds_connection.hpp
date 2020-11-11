#ifndef GDS_CONNECTION_HPP
#define GDS_CONNECTION_HPP

#include "gds_types.hpp"

#include <functional>
#include <memory>
#include <string>

namespace gds_lib {
namespace connection {

    class not_implemented_error : public std::logic_error
    {
    private:
        std::string _text;

    public:

        not_implemented_error(const char* message, const char* function)
            :
            std::logic_error("Not Implemented")
        {
            _text = message;
            _text += " : ";
            _text += function;
        };

        virtual const char *what() const throw()
        {
            return _text.c_str();
        }
    };

    class connection_error : public std::runtime_error
    {
        std::string _message;
    public:
        connection_error(const std::string& message) : std::runtime_error(message), _message(message){}
        virtual const char *what() const throw()
        {
            return _message.c_str();
        }
    };


    struct GDSMessageListener {
        //using gds_message_t = std::shared_ptr<GdsMessage>;

        virtual ~GDSMessageListener(){}

        virtual void on_connection_success(gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>){}
        virtual void on_disconnect(){}
        virtual void on_connection_failure(const std::optional<connection_error>&, std::optional<std::pair<gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>>>){
            throw not_implemented_error{"Connection failure handler was not overridden!", "GDSMessageListener::on_connection_failure()"};
        }

        virtual void on_event_ack3(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsEventReplyMessage> event_ack){
            throw not_implemented_error{"Received an Event ACK 3 but the method was not overridden!", "GDSMessageListener::on_event_ack3()"};
        }

        virtual void on_attachment_request4(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestMessage> request){
            throw not_implemented_error{"Received an Attachment Request 4 but the method was not overridden!", "GDSMessageListener::on_attachment_request4()"};
        }

        virtual void on_attachment_request_ack5(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage> request_ack){
            throw not_implemented_error{"Received an Attachment Request ACK 5 but the method was not overridden!", "GDSMessageListener::on_attachment_request_ack5()"};
        }

        virtual void on_attachment_response6(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage> response){
            throw not_implemented_error{"Received an Attachment Response 6 but the method was not overridden!", "GDSMessageListener::on_attachment_response6()"};
        }

       virtual void on_attachment_response_ack7(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseResultMessage> response_ack){
            throw not_implemented_error{"Received an Attachment Response ACK 7 but the method was not overridden!", "GDSMessageListener::on_attachment_response_ack7()"};
        }

        virtual void on_event_document8(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsEventDocumentMessage> event_document){
            throw not_implemented_error{"Received an Event Document 8 but the method was not overridden!", "GDSMessageListener::on_event_document8()"};
        }

        virtual void on_event_document_ack9(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsEventDocumentReplyMessage> event_document_ack){
            throw not_implemented_error{"Received an Event Document ACK 9 but the method was not overridden!", "GDSMessageListener::on_event_document_ack9()"};
        }

        virtual void on_query_request_ack11(gds_lib::gds_types::gds_message_t, std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage> query_ack){
            throw not_implemented_error{"Received a Query Reply ACK 11 but the method was not overridden!", "GDSMessageListener::on_query_request_ack11()"};
        }
    };


    enum class State : int{
        NOT_CONNECTED,
        INITIALIZING,
        CONNECTING,
        CONNECTED,
        LOGGING_IN,
        LOGGED_IN,
        DISCONNECTED,
        FAILED
    };

    class state_error : public std::logic_error
    {
        std::string msg;
    public:
        explicit state_error(const State expected, const State actual, const std::string& fn="") : std::logic_error("Invalid connection state!") {
            msg += std::string("The client state is invalid! Expected state ID: ");
            msg += std::to_string(static_cast<int>(expected));
            msg += std::string(" Actual state ID: ");
            msg += std::to_string(static_cast<int>(actual));

            if(fn.size()) {
                msg += std::string(" - Thrown from: ");
                msg += fn;
            }
        }

        virtual const char* what() const throw()
        {
            return msg.c_str();
        }
    };

    struct GDSInterface {
        virtual ~GDSInterface();

        virtual void close() = 0;
        virtual void start() = 0;
        virtual void send(const gds_lib::gds_types::GdsMessage& msg) = 0;

        virtual State get_state() = 0;

        /*
        std::function<void()> on_open;
        std::function<void(bool,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>)> on_login;
        std::function<void(gds_lib::gds_types::GdsMessage&)> on_message;
        std::function<void(int, const std::string&)> on_close;
        std::function<void(int, const std::string&)> on_error;
        */

        /*
        //NO AUTH
        static std::shared_ptr<GDSInterface> create(const std::string& url,  std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username);

        //PASSWORD AUTH
        static std::shared_ptr<GDSInterface> create(const std::string& url,  std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, const std::string& password);

        //TLS AUTH
        static std::shared_ptr<GDSInterface> create(const std::string& url,  std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks, const std::string& username, const std::string& cert, const std::string& cert_pw);
        */
    };

    class GDSBuilder {
        std::shared_ptr<gds_lib::connection::GDSMessageListener> callbacks;
        std::string password;
        std::string uri;
        std::string username;
        std::pair<std::string, std::string> tls;
        uint64_t timeout;
    public:
        GDSBuilder() : uri("127.0.0.1:8888/gate"), username("user"), timeout(3000) {}

        GDSBuilder& with_callbacks(std::shared_ptr<gds_lib::connection::GDSMessageListener> value){
            callbacks = value;
            return *this;
        }
        
        GDSBuilder& with_password(const std::string& value){
            password = value;
            return *this;
        }

        GDSBuilder& with_uri(const std::string& value){
            uri = value;
            return *this;
        }

        GDSBuilder& with_username(const std::string& value){
            username = value;
            return *this;
        }

        GDSBuilder& with_tls(const std::pair<std::string, std::string>& value){
            tls = value;
            return *this;
        }

        GDSBuilder& with_timeout(const uint64_t value){
            timeout = value;
            return *this;
        }

        std::shared_ptr<GDSInterface> build() const;
    };

} // namespace connection
} // namespace gds_lib

#endif // GDS_CONNECTION_HPP