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

class GDSConsoleClient {
    std::shared_ptr<gds_lib::connection::GDSInterface> mGDSInterface;

    binary_semaphore_t loginReplySemaphore;

    binary_semaphore_t workDone;

    binary_semaphore_t connectionReady;
    binary_semaphore_t connectionClosed;

    ArgParser args;

    unsigned long timeout;
    std::atomic_bool connection_open;

    bool query_all;

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

    void login();
    void send_query(const std::string&);
    void send_next_query();
    void send_event(const std::string&, const std::string&);
    void send_attachment_request(const std::string&);

    void save_message(gds_lib::gds_types::GdsMessage&);
    void save_binary(const std::vector<std::uint8_t>&, std::string&, const std::string&);

    gds_lib::gds_types::GdsMessage create_default_message();

    gds_lib::gds_types::QueryContextDescriptor contextDescriptor;
    void
    handleLoginReply(gds_lib::gds_types::GdsMessage&,
        std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>&);
    void
    handleEventReply(gds_lib::gds_types::GdsMessage&,
        std::shared_ptr<gds_lib::gds_types::GdsEventReplyMessage>&);
    void handleAttachmentRequestReply(
        gds_lib::gds_types::GdsMessage&,
        std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage>&);
    void handleAttachmentResponse(
        gds_lib::gds_types::GdsMessage&,
        std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage>&);
    void
    handleQueryReply(gds_lib::gds_types::GdsMessage&,
        std::shared_ptr<gds_lib::gds_types::GdsQueryReplyMessage>&);

    void onOpen();
    void onClose(int, const std::string&);
    void onMessageReceived(gds_lib::gds_types::GdsMessage&);
    void onError(int, const std::string&);
};

#endif //CONSOLE_CLIENT_HPP
