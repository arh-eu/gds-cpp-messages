#include "gds_types.hpp"

#include <iostream>


template <typename OStream>
static OStream& operator<<(OStream& os, const gds_lib::gds_types::Stringable& str)
{
  os << str.to_string();
  return os;
}


template <typename OStream, typename T>
static OStream& operator<<(OStream& os, const std::optional<T>& opt)
{
  if(opt)
  {
    os << opt.value();
  }
  else
  {
    os << "null";
  }
  return os;
}


template <typename OStream, typename T>
static OStream& operator<<(OStream& os, const std::vector<T>& items)
{
  os << '[';
  auto it = items.begin();
  if(it != items.end())
  {
    os  << '\n' << *it;
    ++it;
    while(it != items.end())
    {
      os << ", " << '\n' << *it;
      ++it;
    }
  }
  os << '\n' << ']';
  return os;
}



template <typename OStream>
static OStream& operator<<(OStream& os, const gds_lib::gds_types::byte_array& items)
{
  os << '<' << items.size() << " bytes>";
  return os;
}

template <typename OStream, typename T, std::size_t N>
static OStream& operator<<(OStream& os, const std::array<T, N>& array){
  os << '[';
  if(!array.empty())
  {
    os  << '\n' << array[0];
    for(size_t ii = 1;ii<array.size();++ii)
    {
      os << ", "  << '\n' << array[ii];
    }
  }
  os << '\n' << ']';
  return os;
}

template <typename OStream, typename K, typename V>
static OStream& operator<<(OStream& os, const std::map<K,V>& items)
{
  os << '{';
  auto it = items.begin();
  if(it != items.end())
  {
    os  << '\n' << it->first << ": " << it->second;
    ++it;
    while(it != items.end())
    {
      os << ", "  << '\n' << it->first << ": " << it->second;
      ++it;
    }
  }
  os << '\n' << '}';
  return os;
}



namespace gds_lib {
  namespace gds_types {

    void GdsMessage::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
      validate();
      packer.pack_array(11);
      packer.pack(userName);
      packer.pack(messageId);
      packer.pack_int64(createTime);
      packer.pack_int64(requestTime);
      if (isFragmented) {
        packer.pack_true();
        packer.pack(firstFragment.value());
        packer.pack(lastFragment.value());
        packer.pack(offset.value());
        packer.pack(fds.value());
      } else {
        packer.pack_false();
    packer.pack_nil(); // first fragmented
    packer.pack_nil(); // last fragmented
    packer.pack_nil(); // offset
    packer.pack_nil(); // full data size
  }
  packer.pack_int32(dataType); // datatype
  if (messageBody) {
    messageBody->pack(packer);
  } else {
    packer.pack_nil();
  }
}

void GdsMessage::unpack(const msgpack::object &object) {
  std::vector<msgpack::object> data = object.as<std::vector<msgpack::object>>();
  userName = data.at(gds_types::GdsHeader::USER).as<std::string>();
  messageId = data.at(gds_types::GdsHeader::ID).as<std::string>();
  createTime = data.at(gds_types::GdsHeader::CREATE_TIME).as<int64_t>();
  requestTime = data.at(gds_types::GdsHeader::REQUEST_TIME).as<int64_t>();
  isFragmented = data.at(gds_types::GdsHeader::FRAGMENTED).as<bool>();
  if (isFragmented) {
    firstFragment =
    data.at(gds_types::GdsHeader::FIRST_FRAGMENT).as<std::string>();
    lastFragment =
    data.at(gds_types::GdsHeader::LAST_FRAGMENT).as<std::string>();
    offset = data.at(gds_types::GdsHeader::OFFSET).as<int32_t>();
    fds = data.at(gds_types::GdsHeader::FULL_DATA_SIZE).as<int32_t>();
  } else {
    firstFragment.reset();
    lastFragment.reset();
    offset.reset();
    fds.reset();
  }
  dataType = data.at(gds_types::GdsHeader::DATA_TYPE).as<int32_t>();

  switch (dataType) {
  case gds_types::GdsMsgType::LOGIN: // Type 0
  messageBody = std::make_shared<GdsLoginMessage>();
  break;
  case gds_types::GdsMsgType::LOGIN_REPLY: // Type 1
  messageBody = std::make_shared<GdsLoginReplyMessage>();
  break;
  case gds_types::GdsMsgType::EVENT: // Type 2
  messageBody = std::make_shared<GdsEventMessage>();
  break;
  case gds_types::GdsMsgType::EVENT_REPLY: // Type 3
  messageBody = std::make_shared<GdsEventReplyMessage>();
  break;
  case gds_types::GdsMsgType::ATTACHMENT_REQUEST: // Type 4
  messageBody = std::make_shared<GdsAttachmentRequestMessage>();
  break;
  case gds_types::GdsMsgType::ATTACHMENT_REQUEST_REPLY: // Type 5
  messageBody = std::make_shared<GdsAttachmentRequestReplyMessage>();
  break;
  case gds_types::GdsMsgType::ATTACHMENT: // Type 6
  messageBody = std::make_shared<GdsAttachmentResponseMessage>();
  break;
  case gds_types::GdsMsgType::ATTACHMENT_REPLY: // Type 7
  messageBody = std::make_shared<GdsAttachmentResponseResultMessage>();
  break;
  case gds_types::GdsMsgType::EVENT_DOCUMENT: // Type 8
  messageBody = std::make_shared<GdsEventDocumentMessage>();
  break;
  case gds_types::GdsMsgType::EVENT_DOCUMENT_REPLY: // Type 9
  messageBody = std::make_shared<GdsEventDocumentReplyMessage>();
  break;
  case gds_types::GdsMsgType::QUERY: // Type 10
  messageBody = std::make_shared<GdsQueryRequestMessage>();
  break;
  case gds_types::GdsMsgType::QUERY_REPLY: // Type 11
  messageBody = std::make_shared<GdsQueryReplyMessage>();
  break;
  case gds_types::GdsMsgType::GET_NEXT_QUERY: // Type 12
  messageBody = std::make_shared<GdsNextQueryRequestMessage>();
  break;
  default:
  messageBody.reset();
  break;
}
if (messageBody) {
  messageBody->unpack(data.at(gds_types::GdsHeader::DATA));
}

validate();
}

void GdsMessage::validate() const {
  if (createTime < 0) {
    throw invalid_message_error(GdsMsgType::HEADER_MESSAGE);
  }
  if (isFragmented) {
    if (!(firstFragment.has_value() && lastFragment.has_value() &&
      offset.has_value() && fds.has_value())) {
      throw invalid_message_error(GdsMsgType::HEADER_MESSAGE);
  }
} else {
  if ((firstFragment.has_value() || lastFragment.has_value() ||
   offset.has_value() || fds.has_value())) {
    throw invalid_message_error(GdsMsgType::HEADER_MESSAGE);
}
}
if (dataType < 0 || dataType > 14) {
  throw invalid_message_error(GdsMsgType::HEADER_MESSAGE);
}
if (messageBody) {
  messageBody->validate();
}
}

std::string GdsMessage::to_string() const
{
  std::stringstream ss;
  ss << std::boolalpha;
  ss << '[' << '\n';
  ss << userName;
  ss << ", "  << '\n' << messageId;
  ss << ", "  << '\n' << createTime;
  ss << ", "  << '\n' << requestTime;
  ss << ", "  << '\n' << isFragmented;
  ss << ", "  << '\n' << firstFragment;
  ss << ", "  << '\n' << lastFragment;
  ss << ", "  << '\n' << offset;
  ss << ", "  << '\n' << fds;
  ss << ", "  << '\n' << dataType;
  if(messageBody)
  {
    ss << ", "  << '\n' << messageBody->to_string();
  }
  else
  {
    ss  << '\n' << ", null";
  }

  ss  << '\n' << ']';
  return ss.str();
}


void GdsFieldValue::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  switch (type) {
    case msgpack::type::NIL:
    packer.pack_nil();
    break;
    case msgpack::type::BOOLEAN:
    as<bool>() ? packer.pack_true() : packer.pack_false();
    break;
    case msgpack::type::POSITIVE_INTEGER:
    packer.pack_uint64(as<uint64_t>());
    break;
    case msgpack::type::NEGATIVE_INTEGER:
    packer.pack_int64(as<int64_t>());
    break;
    case msgpack::type::FLOAT32:
    packer.pack_float(as<float>());
    break;
    case msgpack::type::FLOAT64:
    packer.pack_double(as<double>());
    break;
    case msgpack::type::STR:
    packer.pack(as<std::string>());
    break;
    case msgpack::type::BIN:
    packer.pack(as<byte_array>());
    break;
    case msgpack::type::ARRAY: {
      std::vector<GdsFieldValue> items = as<std::vector<GdsFieldValue>>();
      packer.pack_array(items.size());
      for (auto &obj : items) {
        obj.pack(packer);
      }
    } break;
    case msgpack::type::MAP:
    packer.pack(as<std::map<std::string, std::string>>());
    break;
    default:
    throw invalid_message_error(GdsMsgType::UNKNOWN);
  }
}

void GdsFieldValue::unpack(const msgpack::object &obj) {
  type = obj.type;
  switch (obj.type) {
    case msgpack::type::NIL:
    value = std::nullopt;
    break;
    case msgpack::type::BOOLEAN:
    value = obj.as<bool>();
    break;
    case msgpack::type::POSITIVE_INTEGER:
    value = obj.as<uint64_t>();
    break;
    case msgpack::type::NEGATIVE_INTEGER:
    value = obj.as<int64_t>();
    break;
    case msgpack::type::FLOAT32:
    value = obj.as<float>();
    break;
    case msgpack::type::FLOAT64:
    value = obj.as<double>();
    break;
    case msgpack::type::STR:
    value = obj.as<std::string>();
    break;
    case msgpack::type::BIN:
    value = obj.as<byte_array>();
    break;
    case msgpack::type::ARRAY: {
      std::vector<msgpack::object> items = obj.as<std::vector<msgpack::object>>();
      std::vector<GdsFieldValue>   values;
      values.reserve(items.size());
      for (auto &object : items) {
        GdsFieldValue current;
        current.unpack(object);
        values.emplace_back(current);
      }
      value = values;
    } break;
    case msgpack::type::MAP:
    value = obj.as<std::map<std::string, std::string>>();
    break;
    default:
    throw invalid_message_error(GdsMsgType::UNKNOWN);
  }
}

void GdsFieldValue::validate() const {}


std::string GdsFieldValue::to_string() const {
  std::stringstream ss;
  switch (type) {
    case msgpack::type::NIL:
    ss << "null";
    break;
    case msgpack::type::BOOLEAN:
    ss << (as<bool>() ? "true" : "false");
    break;
    case msgpack::type::POSITIVE_INTEGER:
    ss << as<uint64_t>();
    break;
    case msgpack::type::NEGATIVE_INTEGER:
    ss << as<int64_t>();
    break;
    case msgpack::type::FLOAT32:
    ss << as<float>();
    break;
    case msgpack::type::FLOAT64:
    ss << as<double>();
    break;
    case msgpack::type::STR:
    ss << as<std::string>();
    break;
    case msgpack::type::BIN:
    ss << "<" << as<byte_array>().size() << "bytes>";
    break;
    case msgpack::type::ARRAY: {
      std::vector<GdsFieldValue> items = as<std::vector<GdsFieldValue>>();
      ss << items;
    } break;
    case msgpack::type::MAP:
    {
      auto items = as<std::map<std::string, std::string>>();
      ss << items;
    }
    break;
    default:
    break;
  }
  return ss.str();
}



void EventReplyBody::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  for (auto &eventResult : results) {
    packer.pack_array(4);

    packer.pack_int32(eventResult.status);
    packer.pack(eventResult.notification);

    packer.pack_array(eventResult.fieldDescriptor.size());
    for (auto &descriptor : eventResult.fieldDescriptor) {
      packer.pack_array(descriptor.size());
      for (auto &item : descriptor) {
        packer.pack(item);
      }
    }

    packer.pack_array(eventResult.subResults.size());
    for (auto &subResult : eventResult.subResults) {
      packer.pack_array(6);
      packer.pack_int32(subResult.status);

      if (subResult.id.has_value()) {
        packer.pack(subResult.id.value());
      } else {
        packer.pack_nil();
      }
      if (subResult.tableName.has_value()) {
        packer.pack(subResult.tableName.value());
      } else {
        packer.pack_nil();
      }
      if (subResult.created.has_value()) {
        subResult.created.value() ? packer.pack_true() : packer.pack_false();
      } else {
        packer.pack_nil();
      }
      if (subResult.version.has_value()) {
        packer.pack(subResult.version.value());
      } else {
        packer.pack_nil();
      }
      if (subResult.values.has_value()) {
        packer.pack_array(subResult.values.value().size());
        for (auto &fieldValue : subResult.values.value()) {
          fieldValue.pack(packer);
        }
      } else {
        packer.pack_nil();
      }
    }
  }
}

void EventReplyBody::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> eventResults =
  packer.as<std::vector<msgpack::object>>();

  results.reserve(eventResults.size());
  for (auto &object : eventResults) {
    std::vector<msgpack::object> currentData =
    object.as<std::vector<msgpack::object>>();

    GdsEventResult currentResult;

    currentResult.status = currentData.at(0).as<int32_t>();
    if (!currentData.at(1).is_nil()) {
      currentResult.notification = currentData.at(1).as<std::string>();
    }

    currentResult.fieldDescriptor =
    currentData.at(2).as<std::vector<field_descriptor>>();

    std::vector<msgpack::object> subResults =
    currentData.at(3).as<std::vector<msgpack::object>>();
    currentResult.subResults.reserve(subResults.size());


    for (auto &subResultObj : subResults) {
      std::vector<msgpack::object> subRes =
      subResultObj.as<std::vector<msgpack::object>>();
      EventSubResult currentSubResult;

      currentSubResult.status = subRes.at(0).as<int32_t>();
      
      if (subRes.size()>1 && !subRes.at(1).is_nil()) {
        currentSubResult.id = subRes.at(1).as<std::string>();
      }

      if (subRes.size()>2 && !subRes.at(2).is_nil()) {
        currentSubResult.tableName = subRes.at(2).as<std::string>();
      }

      if (subRes.size()>3 && !subRes.at(3).is_nil()) {
        currentSubResult.created = subRes.at(3).as<bool>();
      }

      if (subRes.size()>4 && !subRes.at(4).is_nil()) {
        currentSubResult.version = subRes.at(4).as<std::string>();
      }

      if (subRes.size()>5 && !subRes.at(5).is_nil()) {
        std::vector<msgpack::object> fieldValues =
        subRes.at(5).as<std::vector<msgpack::object>>();


        currentSubResult.values = std::vector<GdsFieldValue>();
        currentSubResult.values.value().reserve(fieldValues.size());
        for (auto &fieldValue : fieldValues) {
          GdsFieldValue currentGDSFv;
          currentGDSFv.unpack(fieldValue);
          currentSubResult.values.value().emplace_back(currentGDSFv);
        }

        currentResult.subResults.emplace_back(currentSubResult);
      }
    }

    results.emplace_back(currentResult);
  }

  validate();
}

void EventReplyBody::validate() const {
  // skip
}



std::string EventReplyBody::to_string() const
{
  std::stringstream ss;
  ss << results;
  return ss.str();
}


void AttachmentResult::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  size_t map_elements = 4;
  if (meta.has_value()) {
    map_elements += 1;
  }
  if (ttl.has_value()) {
    map_elements += 1;
  }
  if (to_valid.has_value()) {
    map_elements += 1;
  }
  if (attachment.has_value()) {
    map_elements += 1;
  }

  packer.pack_map(map_elements);
  packer.pack("requestids");
  packer.pack(requestIDs);
  packer.pack("ownertable");
  packer.pack(ownerTable);
  packer.pack("attachmentid");
  packer.pack(attachmentID);
  packer.pack("ownerids");
  packer.pack(ownerIDs);

  if (meta.has_value()) {
    packer.pack("meta");
    packer.pack(meta.value());
  }
  if (ttl.has_value()) {
    packer.pack("ttl");
    packer.pack_int64(ttl.value());
  }
  if (to_valid.has_value()) {
    packer.pack("to_valid");
    packer.pack_int64(to_valid.value());
  }
  if (attachment.has_value()) {
    packer.pack("attachment");
    packer.pack(attachment.value());
  }
}

void AttachmentResult::unpack(const msgpack::object &packer) {
  std::map<std::string, msgpack::object> obj =
  packer.as<std::map<std::string, msgpack::object>>();

  requestIDs = obj.at("requestids").as<std::vector<std::string>>();
  ownerTable = obj.at("ownertable").as<std::string>();
  attachmentID = obj.at("attachmentid").as<std::string>();
  ownerIDs = obj.at("ownerids").as<std::vector<std::string>>();

  if (obj.find("meta") != obj.end()) {
    meta = obj.at("meta").as<std::string>();
  }
  if (obj.find("ttl") != obj.end()) {
    ttl = obj.at("ttl").as<int64_t>();
  }
  if (obj.find("to_valid") != obj.end()) {
    to_valid = obj.at("to_valid").as<int64_t>();
  }
  if (obj.find("attachment") != obj.end()) {
    attachment = obj.at("attachment").as<byte_array>();
  }
  validate();
}
void AttachmentResult::validate() const {}


std::string AttachmentResult::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << requestIDs;
  ss << ", "  << '\n' << ownerTable;
  ss << ", "  << '\n' << attachmentID;
  ss << ", "  << '\n' << ownerIDs;
  ss << ", "  << '\n' << meta;
  ss << ", "  << '\n' << ttl;
  ss << ", "  << '\n' << to_valid;
  if(attachment)
  {
    ss << ", "  << '\n' << "<" << attachment.value().size() << " bytes>";
  }
  else
  {
    ss << ", "  << '\n' << attachment;
  }
  ss  << '\n' << ']';
  return ss.str();
}



void AttachmentRequestBody::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack_int32(status);
  result.pack(packer);
  if (waitTime.has_value()) {
    packer.pack_int64(waitTime.value());
  } else {
    packer.pack_nil();
  }
}

void AttachmentRequestBody::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> obj = packer.as<std::vector<msgpack::object>>();
  status = obj.at(0).as<int32_t>();
  result.unpack(obj.at(1));
  if (obj.at(2).is_nil()) {
    waitTime.reset();
  } else {
    waitTime = obj.at(2).as<int64_t>();
  }
  validate();
}
void AttachmentRequestBody::validate() const {}


std::string AttachmentRequestBody::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << status;
  ss << ", " << '\n' << result;
  ss << ", " << '\n' << waitTime;
  ss << '\n' <<  ']';
  return ss.str();
}



void AttachmentResponse::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_map(3);
  packer.pack("requestids");
  packer.pack(requestIDs);
  packer.pack("ownertable");
  packer.pack(ownerTable);
  packer.pack("attachmentid");
  packer.pack(attachmentID);
}

void AttachmentResponse::unpack(const msgpack::object &packer) {
  std::map<std::string, msgpack::object> obj =
  packer.as<std::map<std::string, msgpack::object>>();

  requestIDs = obj.at("requestids").as<std::vector<std::string>>();
  ownerTable = obj.at("ownertable").as<std::string>();
  attachmentID = obj.at("attachmentid").as<std::string>();
  validate();
}
void AttachmentResponse::validate() const {}


std::string AttachmentResponse::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << requestIDs;
  ss << ", "  << '\n' << ownerTable;
  ss << ", "  << '\n' << attachmentID;
  ss << '\n' << ']';
  return ss.str();
}


void AttachmentResponseBody::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(2);
  packer.pack_int32(status);
  result.pack(packer);
}

void AttachmentResponseBody::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> obj = packer.as<std::vector<msgpack::object>>();
  status = obj.at(0).as<int32_t>();
  result.unpack(obj.at(1));
  validate();
}
void AttachmentResponseBody::validate() const { result.validate(); }


std::string AttachmentResponseBody::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << status;
  ss << ", "  << '\n' << result;
  ss  << '\n' << ']';
  return ss.str();
}


void EventDocumentResult::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack_int32(status_code);
  if (notification.has_value()) {
    packer.pack(notification.value());
  } else {
    packer.pack_nil();
  }
  packer.pack_map(0);
}

void EventDocumentResult::unpack(const msgpack::object &object) {
  std::vector<msgpack::object> data = object.as<std::vector<msgpack::object>>();
  status_code = data.at(0).as<int32_t>();
  if (!data.at(1).is_nil()) {
    notification = data.at(1).as<std::string>();
  }
  validate();
}
void EventDocumentResult::validate() const {}


std::string EventDocumentResult::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << status_code;
  ss << ", " << '\n' << notification;
  ss << ", " << '\n' << returnings;
  ss  << '\n' << ']';
  return ss.str();
}


void QueryContextDescriptor::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  packer.pack_array(9);

  packer.pack(scroll_id);
  packer.pack(select_query);
  packer.pack_int64(delivered_hits);
  packer.pack_int64(query_start_time);
  packer.pack(consistency_type);
  packer.pack(last_bucket_id);
  packer.pack_array(2);
  packer.pack(gds_holder.at(0));
  packer.pack(gds_holder.at(1));

  packer.pack_array(field_values.size());
  for (auto &item : field_values) {
    item.pack(packer);
  }

  packer.pack_array(partition_names.size());
  for (auto &item : partition_names) {
    packer.pack(item);
  }
}
void QueryContextDescriptor::unpack(const msgpack::object &object) {
  std::vector<msgpack::object> data = object.as<std::vector<msgpack::object>>();
  scroll_id = data.at(0).as<std::string>();
  select_query = data.at(1).as<std::string>();
  delivered_hits = data.at(2).as<int64_t>();
  query_start_time = data.at(3).as<int64_t>();
  consistency_type = data.at(4).as<std::string>();
  last_bucket_id = data.at(5).as<std::string>();

  std::vector<std::string> gdsholders =
  data.at(6).as<std::vector<std::string>>();
  gds_holder[0] = gdsholders.at(0);
  gds_holder[1] = gdsholders.at(1);

  std::vector<msgpack::object> fieldvalues =
  data.at(7).as<std::vector<msgpack::object>>();
  fieldvalues.clear();
  field_values.reserve(fieldvalues.size());
  for (auto &val : fieldvalues) {
    GdsFieldValue current;
    current.unpack(val);
    field_values.emplace_back(current);
  }
  partition_names = data.at(8).as<std::vector<std::string>>();
}

void QueryContextDescriptor::validate() const {}


std::string QueryContextDescriptor::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << scroll_id ;
  ss << ", "  << '\n' << select_query ;
  ss << ", "  << '\n' << delivered_hits;
  ss << ", "  << '\n' << query_start_time;
  ss << ", "  << '\n' << consistency_type;
  ss << ", "  << '\n' << last_bucket_id;
  ss << ", "  << '\n' << gds_holder ;
  ss << ", "  << '\n' << field_values;
  ss << ", "  << '\n' << partition_names;
  ss << '\n' << ']';
  return ss.str();
}


void QueryReplyBody::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(7);
  packer.pack_int64(numberOfHits);
  packer.pack_int64(filteredHits);
  hasMorePages ? packer.pack_true() : packer.pack_false();
  queryContextDescriptor.pack(packer);
  packer.pack_array(fieldDescriptors.size());
  for (auto &item : fieldDescriptors) {
    packer.pack_array(3);
    for (auto &desc : item) {
      packer.pack(desc);
    }
  }

  packer.pack_array(hits.size());
  for (auto &hit_rows : hits) {
    packer.pack_array(hit_rows.size());
    for (auto &hit : hit_rows) {
      hit.pack(packer);
    }
  }
  packer.pack_int64(totalNumberOfHits);
}

void QueryReplyBody::unpack(const msgpack::object &obj) {
  std::vector<msgpack::object> items = obj.as<std::vector<msgpack::object>>();
  numberOfHits = items.at(0).as<int64_t>();
  filteredHits = items.at(1).as<int64_t>();
  hasMorePages = items.at(2).as<bool>();
  queryContextDescriptor.unpack(items.at(3));
  std::vector<msgpack::object> fielddescriptors =
  items.at(4).as<std::vector<msgpack::object>>();
  fieldDescriptors.reserve(fielddescriptors.size());
  for (auto &item : fielddescriptors) {
    std::vector<std::string> data = item.as<std::vector<std::string>>();
    field_descriptor         desc;
    desc[0] = data.at(0);
    desc[1] = data.at(1);
    desc[2] = data.at(2);
    fieldDescriptors.emplace_back(desc);
  }

  std::vector<msgpack::object> values =
  items.at(5).as<std::vector<msgpack::object>>();
  hits.reserve(values.size());
  for (auto &object : values) {
    std::vector<msgpack::object> row =
    object.as<std::vector<msgpack::object>>();
    std::vector<GdsFieldValue> currenthit;
    currenthit.reserve(row.size());
    for (auto &val : row) {
      GdsFieldValue value;
      value.unpack(val);
      currenthit.emplace_back(value);
    }

    hits.emplace_back(currenthit);
    if(items.size() > 6)
    {
      totalNumberOfHits = items.at(6).as<int64_t>();
    }
  }

  validate();
}
void QueryReplyBody::validate() const {
  if (hits.size() != numberOfHits) {
    throw invalid_message_error(GdsMsgType::QUERY_REPLY);
  }
}


std::string QueryReplyBody::to_string() const
{
  std::stringstream ss;
  ss << std::boolalpha;
  ss << '[' << '\n';
  ss << numberOfHits;
  ss << ", "  << '\n' << filteredHits;
  ss << ", "  << '\n' << hasMorePages;
  ss << ", "  << '\n' << queryContextDescriptor;
  ss << ", "  << '\n' << fieldDescriptors;
  ss << ", "  << '\n' << hits;
  ss << '\n' << ']';
  return ss.str();
}


/*0*/
void GdsLoginMessage::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();

  size_t message_size = 4;
  if(cluster_name.has_value()){
    message_size += 1;
  }

  if(reserved_fields.has_value()){
    message_size += 1;
  }

  packer.pack_array(message_size);
  
  if(cluster_name.has_value()){
    packer.pack(cluster_name.value());
  }

  serve_on_the_same_connection ? packer.pack_true() : packer.pack_false();
  packer.pack_int32(protocol_version_number);
  if (fragmentation_supported) {
    packer.pack_true();
    packer.pack_int32(fragment_transmission_unit.value());
  } else {
    packer.pack_false();
    packer.pack_nil();
  }

  if (reserved_fields.has_value()) {
    packer.pack_array(reserved_fields.value().size());
    for (auto &field : reserved_fields.value()) {
      packer.pack(field);
    }
  }
}

void GdsLoginMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  size_t idx = 0;
  if(data.at(0).type == msgpack::type::STR){
    cluster_name = data.at(idx++).as<std::string>();
  }

  serve_on_the_same_connection = data.at(idx++).as<bool>();
  protocol_version_number = data.at(idx++).as<int32_t>();
  fragmentation_supported = data.at(idx++).as<bool>();
  if (fragmentation_supported) {
    fragment_transmission_unit = data.at(idx++).as<int32_t>();
  } else {
    fragment_transmission_unit.reset();
    idx++;
  }

  if (data.size() > idx) {
    reserved_fields = data.at(idx).as<std::vector<std::string>>();
  } else {
    reserved_fields.reset();
  }

  validate();
}

void GdsLoginMessage::validate() const {
  if (fragmentation_supported != fragment_transmission_unit.has_value()) {
    throw invalid_message_error(GdsMsgType::LOGIN, "fragmentation_flag");
  }

  if (reserved_fields.has_value() && reserved_fields.value().size() == 0) {
    throw invalid_message_error(GdsMsgType::LOGIN, "reserved_fields");
  }
}


std::string GdsLoginMessage::to_string() const
{
  std::stringstream ss;
  ss << std::boolalpha;
  ss << '[' << '\n';
  if(cluster_name){
    ss << cluster_name << ", " << '\n';
  }
  ss << serve_on_the_same_connection;
  ss << ", "  << '\n' << protocol_version_number;
  ss << ", "  << '\n' << fragmentation_supported;
  ss << ", "  << '\n' << fragment_transmission_unit;
  if(reserved_fields)
  {
    ss << ", " << '\n' << reserved_fields;
  }

  ss  << '\n' << ']';
  return ss.str();
}


/*1*/
void GdsLoginReplyMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack_int32(ackStatus);
  if (loginReply) {
    loginReply.value().pack(packer);
  } else if (errorDetails) {
    packer.pack_map(errorDetails.value().size());
    for (auto &pair : errorDetails.value()) {
      packer.pack_int32(pair.first);
      packer.pack(pair.second);
    }
  } else {
    packer.pack_nil();
  }
  if (ackException) {
    packer.pack(ackException);
  } else {
    packer.pack_nil();
  }
}

void GdsLoginReplyMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  ackStatus = data.at(0).as<int32_t>();
  loginReply.reset();
  errorDetails.reset();

  if (200 == ackStatus && data.at(1).type == msgpack::type::ARRAY) {
    GdsLoginMessage loginmessage;
    loginmessage.unpack(data.at(1));
    loginReply = loginmessage;
  } else if (401 == ackStatus && data.at(1).type == msgpack::type::MAP) {
    errorDetails = data.at(1).as<std::map<int32_t, std::string>>();
  }

  if (!data.at(2).is_nil()) {
    ackException = data.at(2).as<std::string>();
  } else {
    ackException.reset();
  }
  validate();
}

void GdsLoginReplyMessage::validate() const {
  if (loginReply.has_value() == errorDetails.has_value()) {
    if (loginReply.has_value()) {
      throw invalid_message_error(type());
    }
  }
}


std::string GdsLoginReplyMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << ackStatus;
  if(loginReply){
    ss << ", "  << '\n' << loginReply;
  }else{
    ss << ", "  << '\n' << errorDetails;
  }
  ss << '\n' << ", " << ackException;
  ss << '\n' << ']';
  return ss.str();
}


/*2*/
void GdsEventMessage::pack(msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack(operations);
  packer.pack(binaryContents);
  packer.pack(priorityLevels);
}

void GdsEventMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  operations = data.at(0).as<std::string>();
  binaryContents = data.at(1).as<std::map<std::string, byte_array>>();
  priorityLevels =
  data.at(2).as<std::vector<std::vector<std::map<int32_t, bool>>>>();
  validate();
}

void GdsEventMessage::validate() const {
  // skip, nothing specific here
}


std::string GdsEventMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << operations;
  ss << ", "  << '\n' << binaryContents;
  ss << ", "  << '\n' << priorityLevels;
  ss  << '\n' << ']';
  return ss.str();
}


/*3*/
void GdsEventReplyMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();

  packer.pack_array(3);
  packer.pack_int32(ackStatus);
  if (reply) {
    reply->pack(packer);
  } else {
    packer.pack_nil();
  }
  if (ackException) {
    packer.pack(ackException);
  } else {
    packer.pack_nil();
  }
}

void GdsEventReplyMessage::unpack(const msgpack::object &packer) {

  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  ackStatus = data.at(0).as<int32_t>();
  if (!data.at(1).is_nil()) {
    EventReplyBody body;
    body.unpack(data.at(1));
    reply = body;
  } else {
    reply.reset();
  }
  if (!data.at(2).is_nil()) {
    ackException = data.at(2).as<std::string>();
  } else {
    ackException.reset();
  }
  validate();
}

void GdsEventReplyMessage::validate() const {
  if(reply){
    reply.value().validate();
  }
}

std::string GdsEventReplyMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << ackStatus;
  ss << ", "  << '\n' << reply;
  ss << ", "  << '\n' << ackException;
  ss  << '\n' << ']';
  return ss.str();
}



std::string EventReplyBody::EventSubResult::to_string() const
{
  std::stringstream ss;
  ss << std::boolalpha;
  ss << '[' << '\n';
  ss << status;
  ss << ", "  << '\n' << id;
  ss << ", "  << '\n' << tableName;
  ss << ", "  << '\n' << created;
  ss << ", "  << '\n' << version;
  ss << ", "  << '\n' << values;
  ss  << '\n' << ']';
  return ss.str();
}


std::string EventReplyBody::GdsEventResult::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << status;
  ss << ", "  << '\n' << notification;
  ss << ", "  << '\n' << fieldDescriptor;
  ss << ", "  << '\n' << subResults;
  ss << '\n'  << ']';
  return ss.str();
}




/*4*/
void GdsAttachmentRequestMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack(request);
}
void GdsAttachmentRequestMessage::unpack(const msgpack::object &obj) {
  request = obj.as<std::string>();
  validate();
}
void GdsAttachmentRequestMessage::validate() const {
  // skip
}

std::string GdsAttachmentRequestMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << request << '\n';
  ss << ']';
  return ss.str();
}


/*5*/
void GdsAttachmentRequestReplyMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack_int32(ackStatus);
  if (ackStatus == GDSStatusCodes::OK) {
    request->pack(packer);
  } else {
    packer.pack_nil();
  }
  if (ackException) {
    packer.pack(ackException);
  } else {
    packer.pack_nil();
  }
}

void GdsAttachmentRequestReplyMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  ackStatus = data.at(0).as<int32_t>();
  if (!data.at(1).is_nil()) {
    request = AttachmentRequestBody();
    request->unpack(data.at(1));
  } else {
    request.reset();
  }
  if (!data.at(2).is_nil()) {
    ackException = data.at(2).as<std::string>();
  } else {
    ackException.reset();
  }

  validate();
}
void GdsAttachmentRequestReplyMessage::validate() const {
  if ((ackStatus == GDSStatusCodes::OK) != (request.has_value())) {
    throw invalid_message_error(type());
  }
}


std::string GdsAttachmentRequestReplyMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << ackStatus;
  ss << ", "  << '\n' << request;
  ss << ", "  << '\n' << ackException;
  ss  << '\n' << ']';
  return ss.str();
}


/*6*/
void GdsAttachmentResponseMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(1);
  result.pack(packer);
}

void GdsAttachmentResponseMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> obj = packer.as<std::vector<msgpack::object>>();
  result.unpack(obj.at(0));
  validate();
}
void GdsAttachmentResponseMessage::validate() const { result.validate(); }


std::string GdsAttachmentResponseMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << result << '\n';
  ss << ']' << '\n';
  return ss.str();
}


/*7*/
void GdsAttachmentResponseResultMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack_int32(ackStatus);

  if (response) {
    response->pack(packer);
  } else {
    packer.pack_nil();
  }

  if (ackException) {
    packer.pack(ackException);
  } else {
    packer.pack_nil();
  }
}

void GdsAttachmentResponseResultMessage::unpack(const msgpack::object &packer) {

  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  ackStatus = data.at(0).as<int32_t>();
  if (!data.at(1).is_nil()) {
    AttachmentResponseBody body;
    body.unpack(data.at(1));
    response = body;
  } else {
    response.reset();
  }

  if (!data.at(2).is_nil()) {
    ackException = data.at(2).as<std::string>();
  } else {
    ackException.reset();
  }
  validate();
}
void GdsAttachmentResponseResultMessage::validate() const {
  if (response) {
    response->validate();
  }
}


std::string GdsAttachmentResponseResultMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << ackStatus;
  ss << ", "  << '\n' << response;
  ss << ", "  << '\n' << ackException;
  ss  << '\n' << ']';
  return ss.str();
}



/*8*/
void GdsEventDocumentMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(4);
  packer.pack(tableName);
  packer.pack_array(fieldDescriptors.size());
  for (auto &item : fieldDescriptors) {
    packer.pack_array(3);
    for (auto &desc : item) {
      packer.pack(desc);
    }
  }

  packer.pack_array(records.size());
  for (auto &hit_rows : records) {
    packer.pack_array(hit_rows.size());
    for (auto &hit : hit_rows) {
      hit.pack(packer);
    }
  }

  packer.pack_map(0);
}

void GdsEventDocumentMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> obj = packer.as<std::vector<msgpack::object>>();
  tableName = obj.at(0).as<std::string>();

  std::vector<msgpack::object> fielddescriptors =
  obj.at(1).as<std::vector<msgpack::object>>();
  fieldDescriptors.reserve(fielddescriptors.size());
  for (auto &item : fielddescriptors) {
    std::vector<std::string> data = item.as<std::vector<std::string>>();
    field_descriptor         desc;
    desc[0] = data.at(0);
    desc[1] = data.at(1);
    desc[2] = data.at(2);
    fieldDescriptors.emplace_back(desc);
  }

  std::vector<msgpack::object> values =
  obj.at(2).as<std::vector<msgpack::object>>();
  records.reserve(values.size());
  for (auto &object : values) {
    std::vector<msgpack::object> row =
    object.as<std::vector<msgpack::object>>();
    std::vector<GdsFieldValue> currenthit;
    currenthit.reserve(row.size());
    for (auto &val : row) {
      GdsFieldValue value;
      value.unpack(val);
      currenthit.emplace_back(value);
    }

    records.emplace_back(currenthit);
  }

  // returnings = obj.at(3).as<std::map<int32_t, std::vector<std::string>>>();
  validate();
}
void GdsEventDocumentMessage::validate() const {
  size_t headerCount = fieldDescriptors.size();
  for (auto &row : records) {
    if (headerCount != row.size()) {
      throw invalid_message_error(type());
    }
  }
}


std::string GdsEventDocumentMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << tableName;
  ss << ", "  << '\n' << fieldDescriptors;
  ss << ", "  << '\n' << records;
  ss << ", "  << '\n' << returnings;
  ss  << '\n' << ']';
  return ss.str();
}



/*9*/
void GdsEventDocumentReplyMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();

  packer.pack_array(3);
  packer.pack_int32(ackStatus);
  if (results) {
    packer.pack_array(results->size());
    for (auto &event : results.value()) {
      event.pack(packer);
    }
  } else {
    packer.pack_nil();
  }

  if (ackException) {
    packer.pack(ackException);
  } else {
    packer.pack_nil();
  }
}

void GdsEventDocumentReplyMessage::unpack(const msgpack::object &packer) {

  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  ackStatus = data.at(0).as<int32_t>();
  if (!data.at(1).is_nil()) {
    std::vector<msgpack::object> items =
    data.at(1).as<std::vector<msgpack::object>>();
    std::vector<EventDocumentResult> _results;
    _results.reserve(items.size());
    for (auto &object : items) {
      EventDocumentResult result;
      result.unpack(object);
      _results.emplace_back(result);
    }
    results = _results;
  } else {
    results.reset();
  }

  if (!data.at(2).is_nil()) {
    ackException = data.at(2).as<std::string>();
  } else {
    ackException.reset();
  }
  validate();
}
void GdsEventDocumentReplyMessage::validate() const {
  // skip
}

std::string GdsEventDocumentReplyMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << ackStatus;
  ss << ", "  << '\n'<< results;
  ss << ", "  << '\n'<< ackException;
  ss  << '\n' << ']';
  return ss.str();
}


/*10*/
void GdsQueryRequestMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  if (queryPageSize.has_value() && queryType.has_value()) {
    packer.pack_array(5);
  } else {
    packer.pack_array(3);
  }
  packer.pack(selectString);
  packer.pack(consistency);
  packer.pack_int64(timeout);

  if (queryPageSize.has_value() && queryType.has_value()) {
    packer.pack_int32(queryPageSize.value());
    packer.pack_int32(queryType.value());
  }
}

void GdsQueryRequestMessage::unpack(const msgpack::object &obj) {
  std::vector<msgpack::object> items = obj.as<std::vector<msgpack::object>>();
  selectString = items.at(0).as<std::string>();
  consistency = items.at(1).as<std::string>();
  timeout = items.at(2).as<int64_t>();
  if (items.size() == 5) {
    queryPageSize = items.at(3).as<int32_t>();
    queryType = items.at(4).as<int32_t>();
  }
  validate();
}

void GdsQueryRequestMessage::validate() const {
  if (queryPageSize.has_value() != queryType.has_value()) {
    throw invalid_message_error(type());
  }
}


std::string GdsQueryRequestMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << selectString;
  ss << ", "  << '\n'<< consistency;
  ss << ", "  << '\n'<< timeout;
  if(queryPageSize && queryType){
    ss << ", "  << '\n'<< queryPageSize.value();
    ss << ", "  << '\n'<< queryType.value();
  }
  ss  << '\n' << ']';
  return ss.str();
}


/*11*/
void GdsQueryReplyMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(3);
  packer.pack_int32(ackStatus);
  if (response) {
    response->pack(packer);
  } else {
    packer.pack_nil();
  }

  if (ackException) {
    packer.pack(ackException);
  } else {
    packer.pack_nil();
  }
}

void GdsQueryReplyMessage::unpack(const msgpack::object &packer) {
  std::vector<msgpack::object> data = packer.as<std::vector<msgpack::object>>();
  ackStatus = data.at(0).as<int32_t>();

  if (!data.at(1).is_nil()) {
    QueryReplyBody body;
    body.unpack(data.at(1));
    response = body;
  } else {
    response.reset();
  }

  if (!data.at(2).is_nil()) {
    ackException = data.at(2).as<std::string>();
  } else {
    ackException.reset();
  }
  validate();
}
void GdsQueryReplyMessage::validate() const {
  if (response) {
    response->validate();
  }
}


std::string GdsQueryReplyMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << ackStatus;
  ss << ", "  << '\n'<< response;
  ss << ", "  << '\n'<< ackException;
  ss << '\n' << ']';
  return ss.str();
}


/*12*/
void GdsNextQueryRequestMessage::pack(
  msgpack::packer<msgpack::sbuffer> &packer) const {
  validate();
  packer.pack_array(2);
  contextDescriptor.pack(packer);
  packer.pack_int64(timeout);
}

void GdsNextQueryRequestMessage::unpack(const msgpack::object &obj) {
  std::vector<msgpack::object> items = obj.as<std::vector<msgpack::object>>();
  contextDescriptor.unpack(items.at(0));
  timeout = items.at(1).as<int64_t>();
  validate();
}
void GdsNextQueryRequestMessage::validate() const {
  // skip
}

std::string GdsNextQueryRequestMessage::to_string() const
{
  std::stringstream ss;
  ss << '[' << '\n';
  ss << contextDescriptor;
  ss << ", "  << '\n' << timeout;
  ss  << '\n' << ']';
  return ss.str();
}


} // namespace gds_types
} // namespace gds_lib
