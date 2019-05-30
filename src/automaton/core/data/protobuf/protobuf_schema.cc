#include "automaton/core/data/protobuf/protobuf_schema.h"

#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/util/json_util.h>

#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/io/io.h"

using google::protobuf::Descriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;

using google::protobuf::DescriptorProto;
using google::protobuf::EnumDescriptorProto;
using google::protobuf::EnumValueDescriptorProto;
using google::protobuf::FieldDescriptorProto;
using google::protobuf::FieldDescriptorProto_Label;
using google::protobuf::FieldDescriptorProto_Type;
using google::protobuf::FileDescriptorProto;

using google::protobuf::Reflection;

using google::protobuf::compiler::Parser;

using google::protobuf::io::IstreamInputStream;
using google::protobuf::io::Tokenizer;

using google::protobuf::util::MessageToJsonString;
using google::protobuf::util::JsonStringToMessage;

namespace automaton {
namespace core {
namespace data {
namespace protobuf {

/**
This is helper class which is used while parsing proto file.
**/
class io_error_collector : public google::protobuf::io::ErrorCollector {
 public:
  io_error_collector() {
  errors_number = 0;
  errors_list = "";
}

  void AddError(int line, int column, const std::string& message) {
    std::cerr << "*Error: line: " << line << " col: " << column << "->"
        << message << std::endl;
    errors_number++;
    errors_list += "Error: line: " + std::to_string(line) + " col: " +
        std::to_string(column) + "->" + message + "\n";
  }

  void AddWarning(int line, int column, const std::string& message) {
    std::cerr << "*Warning: line: " << line << " col: " << column << "->"
        << message << std::endl;
    errors_list += "Warning: line: " + std::to_string(line) + " col: " +
        std::to_string(column) + "->" + message + "\n";
  }

  void clear_errors() {
    errors_number = 0;
    errors_list = "";
  }

  uint32_t get_number_errors() {
    return errors_number;
  }

  std::string get_all_errors() {
    return errors_list;
  }

 private:
  uint32_t errors_number;
  std::string errors_list;
};


// Protobuf schema definition

protobuf_schema::protobuf_schema() {
  file_descriptor_proto.reset(new FileDescriptorProto());
  file_descriptor_proto->set_syntax("proto3");
}

protobuf_schema::protobuf_schema(const std::string& proto_def) {
  io_error_collector io_error_collector_;

  file_descriptor_proto.reset(new FileDescriptorProto());
  std::istringstream stream(proto_def);
  IstreamInputStream is(&stream);
  // VLOG(9) << "Tokenizing .proto file";
  Tokenizer tok(&is, &io_error_collector_);
  if (io_error_collector_.get_number_errors() > 0) {
    std::stringstream msg;
    msg << io_error_collector_.get_all_errors();
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  Parser parser;
  parser.RecordErrorsTo(&io_error_collector_);
  // VLOG(9) << "Parsing tokenized proto";
  parser.Parse(&tok, file_descriptor_proto.get());
  if (io_error_collector_.get_number_errors() > 0) {
    std::stringstream msg;
    msg << "Errors while parsing:\n" << io_error_collector_.get_all_errors();
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  // VLOG(9) << "Parsing complete";
}

protobuf_schema::~protobuf_schema() {}

FileDescriptorProto* protobuf_schema::get_file_descriptor_proto() {
  return file_descriptor_proto.get();
}

void protobuf_schema::add_dependency(const std::string& schema_name) {
  LOG(INFO) << "Adding dependency on " << schema_name << " to " << file_descriptor_proto->name();
  file_descriptor_proto->add_dependency(schema_name);
}

uint32_t protobuf_schema::create_message(const std::string& message_name) {
  messages.push_back(new DescriptorProto());
  messages[messages.size() - 1]->set_name(message_name);
  return messages.size() - 1;
}

uint32_t protobuf_schema::create_enum(const std::string& enum_name) {
  enums.push_back(new EnumDescriptorProto());
  enums[enums.size() - 1]->set_name(enum_name);
  return enums.size() - 1;
}

void protobuf_schema::add_enum_value(uint32_t enum_id, const std::string& value_name,
    int32_t value) {
  if (enum_id >= enums.size()) {
    std::stringstream msg;
    msg << "No enum with id: " << enum_id;
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::out_of_range(msg.str());
  }
  EnumDescriptorProto* edp = enums[enum_id];
  if (edp == nullptr) {
    std::stringstream msg;
    msg << "Enum descriptor proto is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  EnumValueDescriptorProto* field = edp->add_value();
  field->set_name(value_name);
  field->set_number(value);
}

void protobuf_schema::add_nested_message(int32_t message_id, uint32_t sub_message_id) {
  CHECK_BOUNDS(message_id, 0, messages.size() - 1) << "message_id out of bounds";
  CHECK_BOUNDS(sub_message_id, 0, messages.size() - 1) << "sub_message_id out of bounds";
  if (messages[message_id] == nullptr || messages[sub_message_id] == nullptr) {
    std::stringstream msg;
    msg << "Message is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  DescriptorProto* dpr = messages[message_id];
  DescriptorProto* m = dpr->add_nested_type();
  m->CopyFrom(*messages[sub_message_id]);
}

void protobuf_schema::add_message(int32_t message_id) {
  CHECK_BOUNDS(message_id, 0, messages.size() - 1) << "message_id out of bounds";
  if (messages[message_id] == nullptr) {
    std::stringstream msg;
    msg << "Message is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  DescriptorProto* m = file_descriptor_proto->add_message_type();
  m->CopyFrom(*messages[message_id]);
}

void protobuf_schema::add_enum(uint32_t enum_id, int32_t message_id) {
  if (enum_id >= enums.size()) {
    std::stringstream msg;
    msg << "No enum with id: " << enum_id;
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::out_of_range(msg.str());
  }
  if (enums[enum_id] == nullptr) {
    std::stringstream msg;
    msg << "Enum descriptor proto is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  if (message_id == -1) {
    EnumDescriptorProto* edp = file_descriptor_proto->add_enum_type();
    edp->CopyFrom(*enums[enum_id]);
  } else {
    if (message_id < 0 || static_cast<uint32_t>(message_id) >= messages.size()) {
      std::stringstream msg;
      msg << "No message with id: " << message_id;
      LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::out_of_range(msg.str());
    }
    if (messages[message_id] == nullptr) {
      std::stringstream msg;
      msg << "Message is NULL";
      LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::runtime_error(msg.str());
    }
    DescriptorProto* dpr = messages[message_id];
    EnumDescriptorProto* edp = dpr->add_enum_type();
    edp->CopyFrom(*enums[enum_id]);
  }
}

void protobuf_schema::add_scalar_field(schema::field_info field, int32_t message_id) {
  CHECK_BOUNDS(message_id, 0, messages.size() - 1) << "message_id out of bounds";
  if (field.type == schema::message_type ||
      field.type == schema::enum_type ||
      field.type == schema::unknown) {
      std::stringstream msg;
      msg << "Field type is not scalar!";
      LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::invalid_argument(msg.str());
  }
  DescriptorProto* dpr = messages[message_id];
  if (dpr == nullptr) {
    std::stringstream msg;
    msg << "Descriptor proto is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  FieldDescriptorProto* field_proto = dpr->add_field();
  field_proto->set_name(field.name);
  field_proto->set_type(protobuf_factory::type_to_protobuf_type.at(field.type));
  field_proto->set_number(field.tag);
  if (field.is_repeated) {
    field_proto->set_label(FieldDescriptorProto_Label::FieldDescriptorProto_Label_LABEL_REPEATED);
  }
}

void protobuf_schema::add_enum_field(schema::field_info field, int32_t message_id) {
  CHECK_BOUNDS(message_id, 0, messages.size() - 1) << "message_id out of bounds";
  if (field.type != schema::enum_type) {
    std::stringstream msg;
    msg << "Field is not enum!";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::invalid_argument(msg.str());
  }
  DescriptorProto* dpr = messages[message_id];
  if (dpr == nullptr) {
    std::stringstream msg;
    msg << "Descriptor proto is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  FieldDescriptorProto* field_proto = dpr->add_field();
  field_proto->set_name(field.name);
  field_proto->set_type(FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_ENUM);
  field_proto->set_number(field.tag);
  field_proto->set_type_name(field.fully_qualified_type);
  if (field.is_repeated) {
    field_proto->set_label(FieldDescriptorProto_Label::FieldDescriptorProto_Label_LABEL_REPEATED);
  }
}

void protobuf_schema::add_message_field(schema::field_info field, int32_t message_id) {
  CHECK_BOUNDS(message_id, 0, messages.size() - 1) << "message_id out of bounds";
  if (field.type != schema::message_type) {
    std::stringstream msg;
    msg << "Field type is not message";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::invalid_argument(msg.str());;
  }
  DescriptorProto* dpr = messages[message_id];
  if (dpr == nullptr) {
    std::stringstream msg;
    msg << "Descriptor proto is NULL";
    LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }
  FieldDescriptorProto* field_proto = dpr->add_field();
  field_proto->set_name(field.name);
  field_proto->set_type(
      FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_MESSAGE);
  field_proto->set_number(field.tag);
  field_proto->set_type_name(field.fully_qualified_type);
  if (field.is_repeated) {
    field_proto->set_label(
        FieldDescriptorProto_Label::FieldDescriptorProto_Label_LABEL_REPEATED);
  }
}

/**
  Serializes schema to JSON string.
*/
bool protobuf_schema::to_json(std::string* output) const {
  CHECK_NOTNULL(output);
  CHECK_NOTNULL(file_descriptor_proto);
  auto status = MessageToJsonString(*file_descriptor_proto, output);
  if (!status.ok()) {
    // TODO(asen): Needs better error handling
    std::cout << status.error_message() << std::endl;
  }
  return status.ok();
}

/**
  Deserializes schema from JSON string.
*/
bool protobuf_schema::from_json(const std::string& input) {
  CHECK_NOTNULL(file_descriptor_proto);
  auto status = JsonStringToMessage(input, file_descriptor_proto.get());
  if (!status.ok()) {
    // TODO(asen): Needs better error handling
    std::cout << status.error_message() << std::endl;
  }
  return status.ok();
}

void dump_msg(const DescriptorProto dpr, std::ostream& ostream_, const std::string& prefix) {
  ostream_ << prefix << "message " << dpr.name() << " {" << std::endl;
  std::string new_prefix = '\t' + prefix;
  for (int i = 0; i < dpr.field_size(); ++i) {
    const FieldDescriptorProto fdp = dpr.field(i);
    ostream_ << new_prefix;
    if (fdp.label() == google::protobuf::FieldDescriptorProto_Label_LABEL_REPEATED) {
      ostream_ << "repeated ";
    }
    switch (fdp.type()) {
      case google::protobuf::FieldDescriptorProto_Type_TYPE_INT32:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_SINT32:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_SFIXED32: ostream_ << "int32 "; break;

      case google::protobuf::FieldDescriptorProto_Type_TYPE_INT64:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_SINT64:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_SFIXED64: ostream_ << "int64 "; break;

      case google::protobuf::FieldDescriptorProto_Type_TYPE_UINT32:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_FIXED32: ostream_ << "uint32 "; break;

      case google::protobuf::FieldDescriptorProto_Type_TYPE_UINT64:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_FIXED64: ostream_ << "uint64 "; break;

      case google::protobuf::FieldDescriptorProto_Type_TYPE_BOOL: ostream_ << "boolean "; break;
      case google::protobuf::FieldDescriptorProto_Type_TYPE_STRING:
      case google::protobuf::FieldDescriptorProto_Type_TYPE_BYTES: ostream_ << "blob "; break;
      case google::protobuf::FieldDescriptorProto_Type_TYPE_ENUM: ostream_ << "enum "; break;

      case google::protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE: ostream_ << "message "; break;
      default: break;
    }
    ostream_ << fdp.name() << " = " << fdp.number() << std::endl;
  }
  for (int i = 0; i < dpr.enum_type_size(); ++i) {
    const EnumDescriptorProto edp = dpr.enum_type(i);
    ostream_ << new_prefix << "enum " << edp.name() << " {" << std::endl;
    for (int j = 0; j < edp.value_size(); ++j) {
      const EnumValueDescriptorProto evdp = edp.value(j);
      ostream_ << new_prefix << '\t' << evdp.name() << " = " << evdp.number() << std::endl;
    }
    ostream_ << new_prefix << "}" << std::endl;
  }
  for (int i = 0; i < dpr.nested_type_size(); ++i) {
    dump_msg(dpr.nested_type(i), ostream_, new_prefix);
  }
  ostream_ << prefix << "}" << std::endl;
}

std::string protobuf_schema::dump_schema() {
  auto& fdp = file_descriptor_proto;
  std::stringstream ostream_;
  for (int i = 0; i < fdp->message_type_size(); ++i) {
    const DescriptorProto d = fdp->message_type(i);
    dump_msg(d, ostream_, "");
  }
  for (int i = 0; i < fdp->enum_type_size(); ++i) {
    const EnumDescriptorProto edp = fdp->enum_type(i);
    ostream_ << "enum " << edp.name() << " {" << std::endl;
    for (int j = 0; j < edp.value_size(); ++j) {
      const EnumValueDescriptorProto evdp = edp.value(j);
      ostream_ << '\t' << evdp.name() << " = " << evdp.number() << std::endl;
    }
    ostream_ << "}" << std::endl;
  }
  return ostream_.str();
}

std::vector<std::string> protobuf_schema::get_message_names() {
  std::vector<std::string> result;

  auto& fdp = file_descriptor_proto;
  for (int i = 0; i < fdp->message_type_size(); ++i) {
    const DescriptorProto& d = fdp->message_type(i);
    result.push_back(d.name());
  }

  return result;
}

}  // namespace protobuf
}  // namespace data
}  // namespace core
}  // namespace automaton
