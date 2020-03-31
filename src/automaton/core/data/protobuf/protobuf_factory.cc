#include "automaton/core/data/protobuf/protobuf_factory.h"

#include <sstream>

#include "automaton/core/data/protobuf/protobuf_msg.h"
#include "automaton/core/data/protobuf/protobuf_schema.h"
#include "automaton/core/io/io.h"

using std::string;

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DynamicMessageFactory;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::FieldDescriptorProto_Type;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;
using google::protobuf::Message;

namespace automaton {
namespace core {
namespace data {
namespace protobuf {

/**
This is helper class which is used while parsing proto file.
**/
class proto_error_collector : public
    google::protobuf::DescriptorPool::ErrorCollector {
  uint32_t errors_number;
  std::string errors_list;
 public:
  proto_error_collector();

  /**
    This function is used by protobuf.

      @param filename File name in which the error occurred.
      @param element_name Full name of the erroneous element.
      @param descriptor Descriptor of the erroneous element.
      @param location One of the location constants, above.
      @param message Human-readable error message.
  */
  void AddError(
      const std::string& filename,
      const std::string& element_name,
      const google::protobuf::Message* descriptor,
      google::protobuf::DescriptorPool::ErrorCollector::ErrorLocation location,
      const std::string& message);

  /**
  This function is used by protobuf.

    @param filename File name in which the error occurred.
    @param element_name Full name of the erroneous element.
    @param descriptor Descriptor of the erroneous element.
    @param location One of the location constants, above.
    @param message Human-readable error message.
  */
  void AddWarning(
      const std::string& filename,
      const std::string& element_name,
      const google::protobuf::Message* descriptor,
      google::protobuf::DescriptorPool::ErrorCollector::ErrorLocation location,
      const std::string& message);

  void clear_errors();

  uint32_t get_number_errors() const;

  std::string get_all_errors() const;
};

// Protobuf schema

const std::map<schema::field_type, FieldDescriptorProto_Type>
protobuf_factory::type_to_protobuf_type {
  {schema::blob, FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_STRING},
  {schema::boolean, FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_BOOL},
  {schema::int32, FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_INT32},
  {schema::uint32, FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_UINT32},
  {schema::enum_type, FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_ENUM},
  {schema::message_type, FieldDescriptorProto_Type::FieldDescriptorProto_Type_TYPE_MESSAGE},
};

const std::map<FieldDescriptor::Type, schema::field_type>
protobuf_factory::protobuf_type_to_type {
  {FieldDescriptor::TYPE_STRING, schema::blob},
  {FieldDescriptor::TYPE_BYTES, schema::blob},
  {FieldDescriptor::TYPE_BOOL, schema::boolean},
  {FieldDescriptor::TYPE_INT32, schema::int32},
  {FieldDescriptor::TYPE_SINT32, schema::int32},
  {FieldDescriptor::TYPE_SFIXED32, schema::int32},
  {FieldDescriptor::TYPE_INT64, schema::int64},
  {FieldDescriptor::TYPE_SINT64, schema::int64},
  {FieldDescriptor::TYPE_SFIXED64, schema::int64},
  {FieldDescriptor::TYPE_UINT32, schema::uint32},
  {FieldDescriptor::TYPE_FIXED32, schema::uint32},
  {FieldDescriptor::TYPE_UINT64, schema::uint64},
  {FieldDescriptor::TYPE_FIXED64, schema::uint64},
  {FieldDescriptor::TYPE_ENUM, schema::enum_type},
  {FieldDescriptor::TYPE_MESSAGE, schema::message_type},
};

const std::map<FieldDescriptor::CppType, schema::field_type>
protobuf_factory::protobuf_ccptype_to_type {
  {FieldDescriptor::CPPTYPE_STRING, schema::blob},
  {FieldDescriptor::CPPTYPE_BOOL, schema::boolean},
  {FieldDescriptor::CPPTYPE_INT32, schema::int32},
  {FieldDescriptor::CPPTYPE_UINT32, schema::uint32},
  {FieldDescriptor::CPPTYPE_INT64, schema::int64},
  {FieldDescriptor::CPPTYPE_UINT64, schema::uint64},
  {FieldDescriptor::CPPTYPE_ENUM, schema::enum_type},
  {FieldDescriptor::CPPTYPE_MESSAGE, schema::message_type},
};

protobuf_factory::protobuf_factory() {
  pool = new DescriptorPool();
  dynamic_message_factory = new DynamicMessageFactory(pool);
}

protobuf_factory::~protobuf_factory() {
  delete dynamic_message_factory;
  delete pool;
}

void protobuf_factory::extract_nested_messages(const Descriptor* d) {
  CHECK_NOTNULL(d) << "Message descriptor is nullptr";
  uint32_t num_msg = d->nested_type_count();
  for (uint32_t i = 0; i < num_msg; i++) {
    const Descriptor* desc = d->nested_type(i);
    schemas.push_back(dynamic_message_factory->GetPrototype(desc));
    schemas_names[desc->full_name()] = static_cast<uint32_t>(schemas.size()) - 1;
    extract_nested_messages(desc);
  }
}

void protobuf_factory::extract_nested_enums(const Descriptor* d) {
  CHECK_NOTNULL(d) << "Message descriptor is nullptr";
  uint32_t number_enums = d->enum_type_count();
  for (uint32_t i = 0; i < number_enums; i++) {
    const EnumDescriptor* edesc = d->enum_type(i);
    enums.push_back(edesc);
    enums_names[edesc->full_name()] = static_cast<uint32_t>(enums.size()) - 1;
  }
  uint32_t num_msg = d->nested_type_count();
  for (uint32_t i = 0; i < num_msg; i++) {
    const Descriptor* desc = d->nested_type(i);
    extract_nested_enums(desc);
  }
}

bool protobuf_factory::contain_invalid_data(const Descriptor* d) {
  CHECK_NOTNULL(d) << "Message descriptor is nullptr";
  if (d->oneof_decl_count() > 0) {
    LOG(WARNING) << d->name() << " contains OneOf which is not supported";
    return true;
  }
  uint32_t number_fields = d->field_count();
  for (uint32_t i = 0; i < number_fields; i++) {
    const FieldDescriptor* fd = d->field(i);
    if (fd->is_map()) {
      LOG(WARNING) << d->name() << " contains Map which is not supported";
      return true;
    }
    if (protobuf_type_to_type.find(fd->type()) == protobuf_type_to_type.end()) {
      LOG(WARNING) << d->full_name() << "." << fd->name() << " is of unsupported type";
      return true;
    }
  }
  bool ans = false;
  uint32_t num_msg = d->nested_type_count();
  for (uint32_t i = 0; i < num_msg; i++) {
    const Descriptor* desc = d->nested_type(i);
    ans = ans | contain_invalid_data(desc);
  }
  return ans;
}

void protobuf_factory::import_from_file_proto(FileDescriptorProto* fdp,
                                              const string& name,
                                              const string& package) {
  LOG(INFO) << "Importing schema from file proto name:[" << name << "] package:[" << package << "]";
  CHECK_NOTNULL(fdp) << "File descriptor proto is nullptr";
  fdp->set_package(package);
  fdp->set_name(name);

  // Checks if file with the same name already exists in the pool.
  if (pool->FindFileByName(name) != nullptr) {
    std::stringstream msg;
    msg << "File with name <" << name << "> already exists.";
    LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }

  // Check if all dependencies are imported.
  uint32_t dependencies_number = fdp->dependency_size();
  LOG(INFO) << "Checking dependencies " << dependencies_number;
  for (uint32_t i = 0; i < dependencies_number; ++i) {
    LOG(INFO) << " dependency " << i << " <" << fdp->dependency(i) << ">";
    if (pool->FindFileByName(fdp->dependency(i)) == nullptr) {
      std::stringstream msg;
      msg << "Dependency <" << fdp->dependency(i) << "> was not found. Import it first.";
      LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::runtime_error(msg.str());
    }
  }

  proto_error_collector proto_error_collector_;
  const FileDescriptor* fd = pool->BuildFileCollectingErrors(*fdp, &proto_error_collector_);
  if (proto_error_collector_.get_number_errors() > 0) {
    std::stringstream msg;
    msg << "Errors while parsing:\n" << proto_error_collector_.get_all_errors();
    LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::runtime_error(msg.str());
  }

  LOG(INFO) << "Check for invalid data types";
  // Check for invalid data types
  uint32_t number_messages = fd->message_type_count();
  for (uint32_t i = 0; i < number_messages; i++) {
    const Descriptor* desc = fd->message_type(i);
    if (contain_invalid_data(desc)) {
      std::stringstream msg;
      msg << "Message contains invalid field type! Invalid data in descriptor: " << desc->name();
      LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::runtime_error(msg.str());
    }
  }

  for (uint32_t i = 0; i < number_messages; i++) {
    const Descriptor* desc = fd->message_type(i);
    schemas.push_back(dynamic_message_factory->GetPrototype(desc));
    schemas_names[desc->full_name()] = static_cast<uint32_t>(schemas.size()) - 1;
    extract_nested_messages(desc);
    extract_nested_enums(desc);
  }

  uint32_t number_enums = fd->enum_type_count();
  for (uint32_t i = 0; i < number_enums; i++) {
    const EnumDescriptor* edesc = fd->enum_type(i);
    enums.push_back(edesc);
    enums_names[edesc->full_name()] = static_cast<uint32_t>(enums.size()) - 1;
  }
}

void protobuf_factory::import_schema(schema* schema, const string& name, const string& package) {
  CHECK_NOTNULL(schema) << "schema is nullptr";
  LOG(INFO) << "Importing schema " << name << " with package " << package;
  std::string json;
  schema->to_json(&json);
  // VLOG(9) << json;
  auto pb_schema = dynamic_cast<protobuf_schema*>(schema);
  import_from_file_proto(pb_schema->get_file_descriptor_proto(), name, package);
}

std::string protobuf_factory::dump_message_schema(uint32_t schema_id) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  std::stringstream ostream_;
  const Message* m = schemas[schema_id];
  const Descriptor* desc = m->GetDescriptor();
  ostream_ << "MessageType: " << desc->full_name() << " {" << std::endl;
  ostream_ << "Fields: " << std::endl;
  uint32_t field_count = desc->field_count();
  for (uint32_t i = 0; i < field_count; i++) {
    const FieldDescriptor* fd = desc->field(i);
    ostream_ << "\n\tName: " << fd->name() << std::endl;
    ostream_ << "\tType: ";
    if (fd->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      ostream_ << "message type: " << fd->message_type()->full_name() <<
          std::endl;
    } else if (fd->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      ostream_ << "enum type: " << fd->enum_type()->full_name() << std::endl;
    } else {
      ostream_ << protobuf_type_to_type.at(fd->type()) << std::endl;
    }
    ostream_ << "\tRepeated: ";
    if (fd->is_repeated()) {
      ostream_ << "YES" << std::endl;
    } else {
      ostream_ << "NO" << std::endl;
    }
    ostream_ << "\tTag: " << fd->number() << std::endl << std::endl;
  }
  ostream_ << "\n}" << std::endl;
  return ostream_.str();
}

std::unique_ptr<msg> protobuf_factory::new_message_by_id(uint32_t schema_id) {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  Message* m = schemas[schema_id]->New();
  return std::unique_ptr<msg>(new protobuf_msg(m, this, schema_id));
}

std::unique_ptr<msg> protobuf_factory::new_message_by_name(const char* schema_name) {
  return new_message_by_id(get_schema_id(schema_name));
}

uint32_t protobuf_factory::get_schemas_number() const {
  return static_cast<uint32_t>(schemas.size());
}

uint32_t protobuf_factory::get_enums_number() const {
  return static_cast<uint32_t>(enums.size());
}

uint32_t protobuf_factory::get_enum_id(const string& enum_name) const {
  if (enums_names.find(enum_name) == enums_names.end()) {
    std::stringstream msg;
    msg << "No enum '" << enum_name << '\'';
    LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::invalid_argument(msg.str());
  }
  return enums_names.at(enum_name);
}

std::string protobuf_factory::dump_enum(uint32_t enum_id) const {
  CHECK_LT(enum_id, enums.size());
  CHECK_NOTNULL(enums[enum_id]);
  std::stringstream ostream_;
  const EnumDescriptor* edesc = enums[enum_id];
  ostream_ << edesc->full_name() << " {" << std::endl;
  uint32_t values = edesc->value_count();
  for (uint32_t i = 0; i < values; i++) {
    const EnumValueDescriptor* evdesc = edesc->value(i);
    ostream_ << '\t' << evdesc->name() << " = " << evdesc->number() <<
        std::endl;
  }
  ostream_ << "}" << std::endl;
  return ostream_.str();
}

int32_t protobuf_factory::get_enum_value(uint32_t enum_id, const string& value_name) const {
  CHECK_LT(enum_id, enums.size());
  CHECK_NOTNULL(enums[enum_id]);
  const EnumValueDescriptor* evd = enums[enum_id]->FindValueByName(value_name);
  if (evd == nullptr) {
    std::stringstream msg;
    msg << "No enum value " << value_name;
    LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::invalid_argument(msg.str());
  }
  return evd->number();
}

std::vector<std::pair<string, int32_t> > protobuf_factory::get_enum_values(uint32_t enum_id) const {
  CHECK_LT(enum_id, enums.size());
  CHECK_NOTNULL(enums[enum_id]);
  std::vector<std::pair<string, int32_t> > result;
  const EnumDescriptor* edesc = enums[enum_id];
  uint32_t values = edesc->value_count();
  for (uint32_t i = 0; i < values; i++) {
    const EnumValueDescriptor* evdesc = edesc->value(i);
    result.push_back(std::make_pair(evdesc->name(), evdesc->number()));
  }
  return result;
}

uint32_t protobuf_factory::get_fields_number(uint32_t schema_id) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const Descriptor* desc = schemas[schema_id]->GetDescriptor();
  return desc->field_count();
}

bool protobuf_factory::is_repeated(uint32_t schema_id, uint32_t field_tag) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const FieldDescriptor* fdesc =
      schemas[schema_id]->GetDescriptor()->FindFieldByNumber(field_tag);
  if (fdesc) {
    return fdesc->is_repeated();
  }
  std::stringstream msg;
  msg << "No field with tag: " << field_tag;
  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
  throw std::invalid_argument(msg.str());
}

uint32_t protobuf_factory::get_nested_messages_number(uint32_t schema_id) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const Descriptor* d = schemas[schema_id]->GetDescriptor();
  return d->nested_type_count();
}

uint32_t protobuf_factory::get_nested_message_schema_id(uint32_t schema_id, uint32_t index) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const Descriptor* d = schemas[schema_id]->GetDescriptor();
  const Descriptor* desc = d->nested_type(index);
  return get_schema_id(desc->full_name());
}

schema::field_info protobuf_factory::get_field_info(uint32_t schema_id, uint32_t index) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const Descriptor* desc = schemas[schema_id]->GetDescriptor();
  if (index >= static_cast<uint32_t>(desc->field_count())) {
    std::stringstream msg;
    msg << "No field with such index: " << index;
    LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::out_of_range(msg.str());
  }
  const FieldDescriptor* fdesc = desc->field(index);
  schema::field_type type;
  string full_type = "";
  if (fdesc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    full_type = fdesc->message_type()->full_name();
    type = schema::message_type;
  } else if (fdesc->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    full_type = fdesc->enum_type()->full_name();
    type = schema::enum_type;
  } else {
    type = protobuf_ccptype_to_type.at(fdesc->cpp_type());
  }
  return schema::field_info(
      fdesc->number(),
      type,
      fdesc->name(),
      full_type,
      fdesc->is_repeated());
}

uint32_t protobuf_factory::get_schema_id(const string& message_name) const {
  if (schemas_names.find(message_name) == schemas_names.end()) {
    std::stringstream msg;
    msg << "No schema '" << message_name << '\'';
    LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
    throw std::invalid_argument(msg.str());
  }
  return schemas_names.at(message_name);
}

string protobuf_factory::get_schema_name(uint32_t schema_id) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  return schemas[schema_id]->GetTypeName();
}

string protobuf_factory::get_field_type(uint32_t schema_id, uint32_t tag) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const FieldDescriptor* fdesc = schemas[schema_id]->GetDescriptor()->FindFieldByNumber(tag);
  if (fdesc) {
    std::string res = fdesc->cpp_type_name();
    if (!res.compare("string")) {
      return "blob";
    } else if (!res.compare("bool")) {
      return "boolean";
    }
    return res;
  }
  std::stringstream msg;
  msg << "No field with tag: " << tag;
  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
  throw std::invalid_argument(msg.str());
}

string protobuf_factory::get_message_field_type(uint32_t schema_id, uint32_t field_tag) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const FieldDescriptor* fdesc = schemas[schema_id]->GetDescriptor()->FindFieldByNumber(field_tag);
  if (fdesc) {
    if (fdesc->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
      std::stringstream msg;
      msg << "Field is not message!";
      LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::invalid_argument(msg.str());
    }
    return fdesc->message_type()->full_name();
  }
  std::stringstream msg;
  msg << "No field with tag: " << field_tag;
  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
  throw std::invalid_argument(msg.str());
}

string protobuf_factory::get_enum_field_type(uint32_t schema_id, uint32_t field_tag) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const FieldDescriptor* fdesc = schemas[schema_id]->GetDescriptor()->FindFieldByNumber(field_tag);
  if (fdesc) {
    if (fdesc->cpp_type() != FieldDescriptor::CPPTYPE_ENUM) {
      std::stringstream msg;
      msg << "Field is not enum!";
      LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::invalid_argument(msg.str());
    }
    return fdesc->enum_type()->full_name();
  }
  std::stringstream msg;
  msg << "No field with tag: " << field_tag;
  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
  throw std::invalid_argument(msg.str());
}

uint32_t protobuf_factory::get_field_tag(uint32_t schema_id, const string& name) const {
  CHECK_LT(schema_id, schemas.size());
  CHECK_NOTNULL(schemas[schema_id]);
  CHECK_NOTNULL(schemas[schema_id]->GetDescriptor());
  const FieldDescriptor* fdesc =
      schemas[schema_id]->GetDescriptor()->FindFieldByName(name);
  if (fdesc) {
    return fdesc->number();
  }
  std::stringstream msg;
  msg << "No field with name: " << name;
  LOG(WARNING) << msg.str() << '\n' << el::base::debug::StackTrace();
  throw std::invalid_argument(msg.str());
}

// Error collectors helper classes

proto_error_collector::proto_error_collector() {
  errors_number = 0;
  errors_list = "";
}

void proto_error_collector::AddError(
    const string& schema,  // File name in which the error occurred.
    const string& element_name,  // Full name of the erroneous element.
    const Message* descriptor,  // Descriptor of the
        // erroneous element.
    ErrorLocation location,  // One of the location constants, above.
    const string& message) {  // Human-readable error message.
  LOG(WARNING) << "*Error: schema: " << schema << " <" << element_name
      << "> error message:" << message << std::endl;
  errors_number++;
  errors_list += "Error: schema: " + schema + " <" + element_name +
      "> error message:" + message + "\n";
}

void proto_error_collector::AddWarning(const string& schema,  // File
      // name in which the error occurred.
    const string& element_name,  // Full name of the erroneous element.
    const Message* descriptor,  // Descriptor of the erroneous
        // element.
    ErrorLocation location,  // One of the location constants, above.
    const string& message) {  // Human-readable error message.
  LOG(WARNING) << "*Warning: schema: " << schema << " <" <<
      element_name << "> error message:" << message << std::endl;
  errors_number++;
  errors_list += "Warning: schema: " + schema + " <" + element_name
      + "> error message:" + message + "\n";
}

void proto_error_collector::clear_errors() {
  errors_number = 0;
  errors_list = "";
}

uint32_t proto_error_collector::get_number_errors() const {
  return errors_number;
}

string proto_error_collector::get_all_errors() const {
  return errors_list;
}

}  // namespace protobuf
}  // namespace data
}  // namespace core
}  // namespace automaton
