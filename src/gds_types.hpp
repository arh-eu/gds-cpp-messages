#ifndef GDS_TYPES_HPP
#define GDS_TYPES_HPP

#include <any>
#include <cstdint>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <sstream>
#include <vector>

#include <msgpack.hpp>

namespace gds_lib {
namespace gds_types {

    struct Stringable {
        virtual std::string to_string() const { return {}; }
    };

    struct Packable : public Stringable {
        virtual ~Packable() {}
        virtual void pack(msgpack::packer<msgpack::sbuffer>&) const = 0;
        virtual void unpack(const msgpack::object&) = 0;
        virtual void validate() const {}
    };

    /**
 * Gds Header Fields
 */
    struct GdsHeader {
        enum Enum {
            USER = 0,
            ID = 1,
            CREATE_TIME = 2,
            REQUEST_TIME = 3,
            FRAGMENTED = 4,
            FIRST_FRAGMENT = 5,
            LAST_FRAGMENT = 6,
            OFFSET = 7,
            FULL_DATA_SIZE = 8,
            DATA_TYPE = 9,
            DATA = 10
        };
    };

    /**
 * Gds Message Types
 */
    struct GdsMsgType {
        enum Enum : int32_t {
            HEADER_MESSAGE = -1,
            LOGIN = 0,
            LOGIN_REPLY = 1,
            EVENT = 2,
            EVENT_REPLY = 3,
            ATTACHMENT_REQUEST = 4,
            ATTACHMENT_REQUEST_REPLY = 5,
            ATTACHMENT = 6,
            ATTACHMENT_REPLY = 7,
            EVENT_DOCUMENT = 8,
            EVENT_DOCUMENT_REPLY = 9,
            QUERY = 10,
            QUERY_REPLY = 11,
            GET_NEXT_QUERY = 12,
            ROUTING_TABLE_UPDATE = 13, // should be never sent/received
            TRAFFIC_STATISTICS_UPDATE = 14, // should be never sent/received
            UNKNOWN = 99
        };
    };

    struct GDSStatusCodes {
        enum Enum : int32_t {
            OK = 200,
            CREATED = 201,
            ACCEPTED = 202,
            NOT_ACCEPTABLE = 304,
            BAD_REQUEST = 400,
            UNAUTHORIZED = 401,
            FORBIDDEN = 403,
            NOT_FOUND = 404,
            SEMANTIC_ERROR = 406,
            TIMEOUT = 408,
            CONFLICT = 409,
            GONE = 410,
            PRECONDITION_FAILED = 412,
            TOO_MANY_REQUESTS = 429,
            INTERNAL_SERVER_ERROR = 500,
            BANDWIDTH_LIMIT_EXCEEDED = 509,
            NOT_EXTENDED = 510
        };
    };

    /**
 * Gds Reply Message Structure enum
 */
    struct GdsReplyMsg {
        enum Enum { STATUS = 0,
            DATA = 1,
            EXCEPTION = 2 };
    };

    struct GdsMessage : public Packable {
        std::string userName;
        std::string messageId;
        int64_t createTime;
        int64_t requestTime;
        bool isFragmented;
        std::optional<std::string> firstFragment;
        std::optional<std::string> lastFragment;
        std::optional<int32_t> offset;
        std::optional<int32_t> fds;
        int32_t dataType;
        std::shared_ptr<Packable> messageBody;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    class invalid_message_error : public std::exception {
    private:
        GdsMsgType::Enum m_type;
        std::string info;

    public:
        explicit invalid_message_error(GdsMsgType::Enum type,
            std::string inf = "") noexcept
            : m_type(type),
              info(inf) {}

        virtual char const* what() const noexcept override
        {
            std::string msg = "The structure of the message of type " + std::to_string(m_type) + " is invalid!";
            if (!info.empty()) {
                msg += "Additional info: " + info;
            }
            return msg.c_str();
        };
    };

    using byte_array = std::vector<uint8_t>;
    using field_descriptor = std::array<std::string, 3>;

    struct GdsFieldValue : public Packable {
        std::any value;
        msgpack::type::object_type type;
        template <typename T>
        T as() const { return std::any_cast<T>(value); }
        std::string to_string() const override;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
    };

    /*3*/
    struct EventReplyBody : public Packable {
        struct EventSubResult : public Stringable {
            int32_t status;
            std::optional<std::string> id;
            std::optional<std::string> tableName;
            std::optional<bool> created;
            std::optional<int64_t> version;
            std::optional<std::vector<GdsFieldValue> > values;
            std::string to_string() const override;
        };

        struct GdsEventResult : public Stringable {
            int32_t status;
            std::string notification;
            std::vector<field_descriptor> fieldDescriptor;
            std::vector<EventSubResult> subResults;
            std::string to_string() const override;
        };

        std::vector<GdsEventResult> results;
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct AttachmentResult : public Packable {
        std::vector<std::string> requestIDs;
        std::string ownerTable;
        std::string attachmentID;
        std::vector<std::string> ownerIDs;
        std::optional<std::string> meta;
        std::optional<int64_t> ttl;
        std::optional<int64_t> to_valid;
        std::optional<byte_array> attachment;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct AttachmentRequestBody : public Packable {
        int32_t status;
        AttachmentResult result;
        std::optional<int64_t> waitTime;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct AttachmentResponse : public Packable {
        std::vector<std::string> requestIDs;
        std::string ownerTable;
        std::string attachmentID;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct AttachmentResponseBody : public Packable {
        int32_t status;
        AttachmentResponse result;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct EventDocumentResult : public Packable {
        int32_t status_code;
        std::optional<std::string> notification;
        std::map<std::string, GdsFieldValue> returnings;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct QueryContextDescriptor : public Packable {
        std::string scroll_id;
        std::string select_query;
        int64_t delivered_hits;
        int64_t query_start_time;
        std::string consistency_type;
        std::string last_bucket_id;
        std::array<std::string, 2> gds_holder;
        std::vector<GdsFieldValue> field_values;
        std::vector<std::string> partition_names;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    struct QueryReplyBody : public Packable {
        int64_t numberOfHits;
        int64_t filteredHits;
        bool hasMorePages;
        QueryContextDescriptor queryContextDescriptor;
        std::vector<field_descriptor> fieldDescriptors;
        std::vector<std::vector<GdsFieldValue> > hits;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    // There's a total of 13 different messages that can be sent - from LOGIN (0) to
    // GET_NEXT_QUERY(12) 13 and 14 should only be sent between GDS instances,
    // therefore clients don't have to support it

    // They all need to be stored somehow in the replymessage object,
    // and since runtime 'instanceof' and/or 'Object' is not available here ->
    // there's pointer and inheritance for this
    // the type() should return the proper enum so it can be used in a a
    // dynamic_cast<> for member access on the proper dynamic types

    struct GdsMessageData : public Packable {
        virtual inline GdsMsgType::Enum type() const noexcept = 0;
    };

    struct GdsACKMessage : public GdsMessageData {
        int32_t ackStatus;
        std::optional<std::string> ackException;
    };

    /*0*/
    struct GdsLoginMessage : public GdsMessageData {
        std::optional<std::string> cluster_name;
        bool serve_on_the_same_connection;
        int32_t protocol_version_number;
        bool fragmentation_supported;
        std::optional<int32_t> fragment_transmission_unit;
        std::optional<std::vector<std::string> > reserved_fields;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::LOGIN;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*1*/
    struct GdsLoginReplyMessage : public GdsACKMessage {
        std::optional<GdsLoginMessage> loginReply;
        std::optional<std::map<int32_t, std::string> > errorDetails;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::LOGIN_REPLY;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*2*/
    struct GdsEventMessage : public GdsMessageData {
        std::string operations;
        std::map<std::string, byte_array> binaryContents;
        std::vector<std::vector<std::map<int32_t, bool> > > priorityLevels;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::EVENT;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*3*/
    struct GdsEventReplyMessage : public GdsACKMessage {
        std::optional<EventReplyBody> reply;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::EVENT_REPLY;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*4*/
    struct GdsAttachmentRequestMessage : public GdsMessageData {
        std::string request;
        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::ATTACHMENT_REQUEST;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*5*/
    struct GdsAttachmentRequestReplyMessage : public GdsACKMessage {
        std::optional<AttachmentRequestBody> request;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::ATTACHMENT_REQUEST_REPLY;
        }
    };

    /*6*/
    struct GdsAttachmentResponseMessage : public GdsMessageData {
        AttachmentResult result;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::ATTACHMENT;
        }
    };

    /*7*/
    struct GdsAttachmentResponseResultMessage : public GdsACKMessage {
        std::optional<AttachmentResponseBody> response;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::ATTACHMENT_REPLY;
        }
    };

    /*8*/
    struct GdsEventDocumentMessage : public GdsMessageData {
        std::string tableName;
        std::vector<field_descriptor> fieldDescriptors;
        std::vector<std::vector<GdsFieldValue> > records;
        std::map<int32_t, std::vector<std::string> > returnings;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::EVENT_DOCUMENT;
        }
    };

    /*9*/
    struct GdsEventDocumentReplyMessage : public GdsACKMessage {
        std::optional<std::vector<EventDocumentResult> > results;

        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::EVENT_DOCUMENT_REPLY;
        }
    };

    /*10*/
    struct GdsQueryRequestMessage : public GdsMessageData {
        std::string selectString;
        std::string consistency;
        int64_t timeout;
        std::optional<int32_t> queryPageSize;
        std::optional<int32_t> queryType;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::QUERY;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*11*/
    struct GdsQueryReplyMessage : public GdsACKMessage {
        std::optional<QueryReplyBody> response;

        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::QUERY_REPLY;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

    /*12*/
    struct GdsNextQueryRequestMessage : public GdsMessageData {
        QueryContextDescriptor contextDescriptor;
        int64_t timeout;
        inline GdsMsgType::Enum type() const noexcept override
        {
            return GdsMsgType::GET_NEXT_QUERY;
        }
        void pack(msgpack::packer<msgpack::sbuffer>&) const override;
        void unpack(const msgpack::object&) override;
        void validate() const override;
        std::string to_string() const override;
    };

} // namespace gds_types
} // namespace gds_lib
#endif // GDS_TYPES_HPP
