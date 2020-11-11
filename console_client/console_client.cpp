#include "console_client.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <utility>
#include <vector>

#include "gds_uuid.hpp"

using namespace gds_lib;
using namespace gds_lib::gds_types;

GDSConsoleClient::GDSConsoleClient(const ArgParser& _args)
    : args(_args), login_success(false)
{
    timeout = std::stoul(args.get_arg("timeout")) * 1000;
    std::cout << "Timeout is set to " << args.get_arg("timeout") << " seconds" << std::endl;

}

GDSConsoleClient::~GDSConsoleClient() { close(); }

void GDSConsoleClient::close()
{
    if (mGDSInterface) {
        mGDSInterface->close();
        mGDSInterface.reset();
    }
}

int64_t GDSConsoleClient::now()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void GDSConsoleClient::setupConnection()
{
    std::cout << "Initializing WebSocket connection.." << std::endl;
    mGDSInterface->start();
    if (!connectionReady.wait_for(timeout)) {
        throw std::runtime_error("Timeout passed while waiting for connection setup!");
    }
}

void GDSConsoleClient::run()
{
    std::cout << "Setting up ConsoleClient.." << std::endl;
    gds_lib::connection::GDSBuilder builder;

    builder.with_uri(args.get_arg("url")).with_username(args.get_arg("username")).with_timeout(timeout).with_callbacks(shared_from_this());

    if(args.has_arg("cert") && args.has_arg("secret"))
    {
        builder.with_tls(std::make_pair(args.get_arg("cert"), args.get_arg("secret")));
    }

    if(args.has_arg("password")){
        builder.with_password(args.get_arg("password"));
    }
    mGDSInterface = builder.build();
    setupConnection();

    if(!login_success){
        return;
    }


    if (args.has_arg("query") || args.has_arg("queryall")) {
        std::string query_string = args.has_arg("query") ? args.get_arg("query") : args.get_arg("queryall");
        query_all = args.has_arg("queryall");
        send_query(query_string);
    }
    else if (args.has_arg("event")) {
        std::string event_string = args.get_arg("event");
        std::string file_list;
        if (args.has_arg("files")) {
            file_list = args.get_arg("files");
        }
        send_event(event_string, file_list);
    }
    else if (args.has_arg("attachment")) {
        std::string attach_string = args.get_arg("attachment");
        send_attachment_request(attach_string);
    }

    if (!workDone.wait_for(timeout)) {
        std::cout << "The GDS did not send its message in time!" << std::endl;
    }
}

void GDSConsoleClient::send_query(const std::string& query_str)
{
    GdsMessage fullMessage = create_default_message();
    message_id = fullMessage.messageId;

    fullMessage.dataType = GdsMsgType::QUERY;

    std::shared_ptr<GdsQueryRequestMessage> selectBody(new GdsQueryRequestMessage());
    {
        selectBody->selectString = query_str;
        selectBody->consistency = "PAGES";
        selectBody->timeout = 0;
    }
    fullMessage.messageBody = selectBody;

    std::cout << "Trying to send SELECT query message to the GDS.." << std::endl;
    mGDSInterface->send(fullMessage);
}

void GDSConsoleClient::send_next_query()
{
    GdsMessage fullMessage = create_default_message();

    fullMessage.dataType = GdsMsgType::GET_NEXT_QUERY;

    std::shared_ptr<GdsNextQueryRequestMessage> selectBody(new GdsNextQueryRequestMessage());
    {
        selectBody->contextDescriptor = contextDescriptor;
        selectBody->timeout = 0;
    }
    fullMessage.messageBody = selectBody;

    std::cout << "Trying to send next SELECT query message to the GDS.." << std::endl;
    mGDSInterface->send(fullMessage);
}

void GDSConsoleClient::send_event(const std::string& event_str, const std::string& file_list)
{

    GdsMessage fullMessage = create_default_message();
    fullMessage.dataType = GdsMsgType::EVENT;

    std::shared_ptr<GdsEventMessage> eventBody(new GdsEventMessage());
    {
        eventBody->operations = event_str;
        if (file_list.length() != 0) {
            std::map<std::string, byte_array> binaries;

            std::stringstream file_list_stream(file_list);
            std::string tmp;
            std::vector<std::string> filenames;
            constexpr static char delimiter = ';';
            while (std::getline(file_list_stream, tmp, delimiter)) {
                filenames.emplace_back(tmp);
            }

            for (const auto& filename : filenames) {
                std::string filepath = "attachments/";
                filepath += filename;

                
                std::fstream file(filepath.c_str(), std::ios::binary | std::ios::in);
                if (file.is_open()) {
                    byte_array content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    binaries[GDSConsoleClient::to_hex(filename)] = content;
                    std::cout << "Adding " << filename << " as an attachment.." << std::endl;
                }
                else {
                    std::string error_message = "Could not open the attachment file named '";
                    error_message.append(filename);
                    error_message.append("'!");
                    std::cout << error_message << '\n';
                    throw std::runtime_error(error_message);
                }
            }

            eventBody->binaryContents = binaries;
        }
    }
    fullMessage.messageBody = eventBody;
    std::cout << "Sending the EVENT message to the GDS.." << std::endl;
    mGDSInterface->send(fullMessage);
}

void GDSConsoleClient::send_attachment_request(const std::string& request)
{
    GdsMessage fullMessage = create_default_message();
    message_id = fullMessage.messageId;

    fullMessage.dataType = GdsMsgType::ATTACHMENT_REQUEST;

    std::shared_ptr<GdsAttachmentRequestMessage> requestBody(new GdsAttachmentRequestMessage());
    {
        requestBody->request = request;
    }
    fullMessage.messageBody = requestBody;

    std::cout << "Trying to send attachment request message to the GDS.." << std::endl;
    mGDSInterface->send(fullMessage);
}

gds_lib::gds_types::GdsMessage GDSConsoleClient::create_default_message()
{
    int64_t currentTime = now();
    gds_lib::gds_types::GdsMessage fullMessage;

    fullMessage.userName = args.get_arg("username");
    fullMessage.messageId = uuid::generate_uuid_v4();
    fullMessage.createTime = currentTime;
    fullMessage.requestTime = currentTime;
    fullMessage.isFragmented = false;

    return fullMessage;
}

std::string GDSConsoleClient::to_hex(const std::string& src)
{
    std::stringstream ss;
    for (auto ch : src) {
        ss << std::hex << (int)ch;
    }
    return ss.str();
}

void GDSConsoleClient::on_connection_success(gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>)
{
    login_success = true;
    connectionReady.notify();
}

void GDSConsoleClient::on_event_ack3(gds_lib::gds_types::gds_message_t fullMessage,
    std::shared_ptr<GdsEventReplyMessage> eventReply)
{
    // do whatever you want to do with this.

    std::cout << "CLIENT received an EVENT reply message! ";
    std::cout << "Event reply status code is: " << eventReply->ackStatus
              << std::endl;

    std::cout << "Full message:" << std::endl;
    std::cout << fullMessage->to_string() << std::endl;
    workDone.notify();
}

void GDSConsoleClient::on_attachment_request_ack5(gds_lib::gds_types::gds_message_t,
    std::shared_ptr<gds_lib::gds_types::GdsAttachmentRequestReplyMessage> replyBody)
{
    std::cout << "CLIENT received an AttachmentRequestReply message! "
              << std::endl;
    std::cout << "Global status code is: " << replyBody->ackStatus << std::endl;

    if (replyBody->ackStatus == 200) {
        AttachmentRequestBody& body = replyBody->request.value();
        AttachmentResult& result = body.result;
        if (result.attachment) {
            std::vector<uint8_t>& binary_data = result.attachment.value();
            std::string filename = "attachments/";
            filename += result.attachmentID;

            if (result.meta) {
                std::string mimetype = result.meta.value();
                save_binary(binary_data, filename, mimetype);
            }
            else {
                save_binary(binary_data, filename, "");
            }
            workDone.notify();
        }
        else {
            std::cout << "Attachment not yet received, awaiting.." << std::endl;
        }
    }
    else {
        if (replyBody->ackException) {
            std::cout << replyBody->ackException.value() << std::endl;
        }
        workDone.notify();
    }
}

void GDSConsoleClient::on_attachment_response6(gds_lib::gds_types::gds_message_t,
    std::shared_ptr<gds_lib::gds_types::GdsAttachmentResponseMessage> replyBody)
{
    std::cout << "CLIENT received an AttachmentResponse message! "
              << std::endl;

    AttachmentResult result = replyBody->result;

    std::vector<uint8_t>& binary_data = result.attachment.value();
    std::string filename = "attachments/";
    filename += result.attachmentID;

    if (result.meta) {
        std::string mimetype = result.meta.value();
        save_binary(binary_data, filename, mimetype);
    }
    else {
        save_binary(binary_data, filename, "");
    }

    {
        GdsMessage fullMessage = create_default_message();
        message_id = fullMessage.messageId;

        fullMessage.dataType = GdsMsgType::ATTACHMENT_REPLY;
        std::shared_ptr<GdsAttachmentResponseResultMessage> requestBody(new GdsAttachmentResponseResultMessage());
        {
            requestBody->ackStatus = 200;
            requestBody->response = {};
            requestBody->response->status = 201;
            requestBody->response->result.requestIDs = result.requestIDs;
            requestBody->response->result.ownerTable = result.ownerTable;
            requestBody->response->result.attachmentID = result.attachmentID;
        }
        fullMessage.messageBody = requestBody;

        std::cout << "Send attachment ACK back to the GDS.." << std::endl;
        mGDSInterface->send(fullMessage);
    }

    workDone.notify();
}

void GDSConsoleClient::on_query_request_ack11(gds_lib::gds_types::gds_message_t,
    std::shared_ptr<GdsQueryReplyMessage> queryReply)
{
    std::cout << "CLIENT received a SELECT reply message! ";
    std::cout << "SELECT status code is: " << queryReply->ackStatus << std::endl;

    bool hasMorePages = false;
    if (queryReply->response) {
        hasMorePages = queryReply->response->hasMorePages;
        contextDescriptor = queryReply->response->queryContextDescriptor;
    }

    if (query_all && hasMorePages) {
        send_next_query();
    }
    else {
        workDone.notify();
    }
}

void GDSConsoleClient::save_binary(const std::vector<std::uint8_t>& binary_data, std::string& filename, const std::string& mimetype)
{

    std::filesystem::create_directory("attachments");
    std::cout << "Saving binary attachment.." << std::endl;
    std::string extension = ".unknown";
    if (mimetype == "image/png") {
        extension = ".png";
    }
    else if (mimetype == "image/jpg" || mimetype == "image/jpeg") {
        extension = ".jpg";
    }
    else if (mimetype == "image/bmp") {
        extension = ".bmp";
    }
    else if (mimetype == "video/mp4") {
        extension = ".mp4";
    }

    filename += extension;

    std::FILE* output = fopen(filename.c_str(), "wb");
    if (output) {
        fwrite(binary_data.data(), sizeof(std::uint8_t), binary_data.size(), output);
        fclose(output);
        std::cout << "Attachment saved as '" << filename << "'!" << std::endl;
    }
    else {
        std::cout << "Could not open " + filename + "!" << std::endl;
    }
}

void GDSConsoleClient::save_message(gds_lib::gds_types::GdsMessage& fullMessage)
{
    std::filesystem::create_directory("exports");

    std::string filename = "exports/";
    filename += fullMessage.messageId;
    filename += ".txt";
    std::ofstream output(filename);
    if (output.is_open()) {
        std::cout << "Reply message is saved as " << filename << std::endl;
        output << fullMessage.to_string();
    }
    else {
        std::cout << "Could not open " + filename + "!" << std::endl;
    }
}

void GDSConsoleClient::on_connection_failure(const std::optional<gds_lib::connection::connection_error>& e, 
        std::optional<std::pair<gds_lib::gds_types::gds_message_t,std::shared_ptr<gds_lib::gds_types::GdsLoginReplyMessage>>> reply) {
    std::cerr << "Could not login!" << std::endl;
    if(e){
        std::cerr << e->what() << std::endl;
    }
    connectionReady.notify();
    workDone.notify();
}