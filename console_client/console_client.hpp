#ifndef CONSOLE_CLIENT_HPP
#define CONSOLE_CLIENT_HPP

#include "gds_connection.hpp"
#include "gds_types.hpp"
#include "semaphore.hpp"

#include "arg_parse.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <exception>

class GDSConsoleClient : public gds_lib::connection::GDSMessageListener, public std::enable_shared_from_this<GDSConsoleClient> {
    
    std::shared_ptr<gds_lib::connection::GDSInterface> mGDSInterface;

    binary_semaphore_t workDone;
    binary_semaphore_t connectionReady;

    ArgParser args;

    unsigned long timeout;

    bool query_all;
    bool login_success;
    std::string message_id;

public:
    GDSConsoleClient(const ArgParser&);
    ~GDSConsoleClient();

    void run();
    void close();
    static std::string to_hex(const std::string&);

private:
    void setupConnection();

    int64_t now();

    void send_query(const std::string&);
    void send_next_query();
    void send_event(const std::string&, const std::string&);
    void send_attachment_request(const std::string&);

    void save_message(gds_lib::gds_types::GdsMessage&);
    void save_binary(const std::vector<std::uint8_t>&, std::string&, const std::string&);

    gds_lib::gds_types::GdsMessage create_default_message();

    gds_lib::gds_types::QueryContextDescriptor contextDescriptor;

    virtual void on_connection_success(gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>) override;

    virtual void on_connection_failure(const std::optional<gds_lib::connection::connection_error>&, 
        std::optional<std::pair<gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>>>) override;

    virtual void on_event_ack3(gds_lib::gds_types::gds_message_t,
        std::shared_ptr<gds_lib::gds_types::GdsEventReplyMessage>) override;

    virtual void on_attachment_request_ack5(gds_lib::gds_types::gds_message_t,
        std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage>) override;

    virtual void on_attachment_response6(gds_lib::gds_types::gds_message_t,
        std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage>) override;
    
    virtual void on_query_request_ack11(gds_lib::gds_types::gds_message_t,
        std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage>) override;
};

#endif //CONSOLE_CLIENT_HPP
