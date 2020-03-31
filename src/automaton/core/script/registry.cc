#include "automaton/core/script/registry.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

#include "automaton/core/data/protobuf/protobuf_factory.h"
#include "automaton/core/io/io.h"

using automaton::core::data::factory;
using automaton::core::data::protobuf::protobuf_factory;

namespace automaton {
namespace core {
namespace script {

void module::add_function(const std::string function_name, module_static_function func) {
  if (functions_.count(function_name) > 0) {
    std::stringstream ss;
    ss << "Module " << name() << " already has function named " << function_name;
    LOG(WARNING) << ss.str();
    throw ss.str();
  }

  functions_[function_name] = {function_name, func, 0, 0};
}

void module::bind_schemas() {
  auto& factory = registry::instance().get_factory();

  // Bind static functions
  for (auto& func : functions_) {
    auto input_id =
        factory.get_schema_id(name_with_api_version() + "." + func.first + ".request");

    auto output_id =
        factory.get_schema_id(name_with_api_version() + "." + func.first + ".response");

    LOG(DBUG) << "Binding " << func.first << "<" << input_id << ", " << output_id << ">";
    func.second.output_schema_id = output_id;
    func.second.input_schema_id = input_id;
  }

  for (auto& impl : implementations_) {
    auto full_scope_implementation = name_with_api_version() + "." + impl.first;
    auto constructor_id = factory.get_schema_id(full_scope_implementation);
    LOG(DBUG) << "Binding " << impl.first <<  "<" << constructor_id << ">";
    impl.second.constructor_schema_id = constructor_id;

    for (auto& concept : impl.second.concepts) {
      // Import methods from concept.
      try {
        auto concept_schema_id = factory.get_schema_id(concept.name);
        // Extract methods from nested messages.
        for (uint32_t i = 0; i < factory.get_nested_messages_number(concept_schema_id); i++) {
          auto method_schema_id = factory.get_nested_message_schema_id(concept_schema_id, i);
          auto method_schema_name = factory.get_schema_name(method_schema_id);
          auto last = method_schema_name.rfind('.');
          auto method_name = method_schema_name.substr(last+1);
          if (method_name == "getters") {
            // TODO(asen): Handle getters.
            continue;
          }
          method_info mi;
          mi.name = method_name;
          mi.input_schema_id = factory.get_schema_id(method_schema_name + ".request");
          mi.output_schema_id = factory.get_schema_id(method_schema_name + ".response");;
          concept.methods.push_back(mi);
          LOG(DBUG) << impl.first << "."
              << method_name
              << "<" << mi.input_schema_id
              << ", " << mi.output_schema_id
              << ">";
        }
      } catch (...) {
        std::stringstream ss;
        ss << "Could not locate concept schema for " << concept.name;
        LOG(WARNING) << ss.str();
        throw ss.str();
      }
    }
  }

  // TODO(asen): Bind object constructors
  // TODO(asen): Bind object method calls
}

void module::add_implementation(const std::string implementation,
                                const std::vector<std::string> concepts,
                                constructor_function f) {
  CHECK_GT(concepts.size(), 0) << implementation << " should have a concept!";
  auto& factory = registry::instance().get_factory();

  if (implementations_.count(implementation) > 0) {
    std::stringstream ss;
    ss << "Module " << name() << " already has implementation for " << implementation;
    LOG(WARNING) << ss.str();
    throw ss.str();
  }
  implementation_info info;
  info.name = implementation;
  info.func = f;
  info.constructor_schema_id = 0;
  for (auto& concept : concepts) {
    LOG(DBUG) << implementation << " : concept " << concept;
    concept_info ci;
    ci.name = concept;

    info.concepts.push_back(ci);
  }

  implementations_[implementation] = info;
}

void module::check_implementation(const std::string implementation) {
  auto& factory = registry::instance().get_factory();
  auto full_scope_implementation = name_with_api_version() + "." + implementation;
  try {
    factory.get_schema_id(full_scope_implementation);
  } catch (...) {
    std::stringstream ss;
    ss << "Could not locate constructor message schema for " << full_scope_implementation;
    LOG(WARNING) << ss.str();
    throw ss.str();
  }
}


registry::registry() {
  auto ptr = new protobuf_factory();
  LOG(INFO) << "Created factory " << ptr;
  factory_ = std::unique_ptr<factory>(ptr);
}

std::string registry::to_string() {
  const size_t w1 = 25;
  const size_t w2 = 25;
  const size_t w3 = 30;
  const size_t w = w1 + w2 + w3;

  std::stringstream ss;
  ss << std::endl;
  ss << std::left;
  ss << std::string(w + 7, '=') << std::endl;
  ss << std::setw(w + 6)
     << "= automaton::core::script::registry modules" << "=" << std::endl;
  ss << std::string(w + 7, '=') << std::endl;
  ss << "| " << std::setw(w1) << "module name"
     << "| " << std::setw(w2) << "API + deps"
     << "| " << std::setw(w3) << "full version" << "|" << std::endl;
  ss << "+-" << std::string(w1, '-')
     << "+-" << std::string(w2, '-')
     << "+-" << std::string(w3, '-')
     << "+" << std::endl;
  std::vector<std::string> keys;
  for (const auto& m : modules_) {
    keys.push_back(m.first);
  }
  std::sort(keys.begin(), keys.end());
  for (const auto& k : keys) {
    const auto& m = *(modules_[k]);
    ss << "| " << std::setw(w1) << m.name()
       << "| " << std::setw(w2) << m.name_with_api_version()
       << "| " << std::setw(w3) << m.full_version() << "|" << std::endl;
    const auto& deps = m.dependencies();
    if (deps.size() > 0) {
      for (const auto& dep : deps) {
        ss << "| " << std::setw(w1) << ""
           << "| " << std::setw(w2) << (" - " + dep)
           << "| " << std::setw(w3) << "" << "|" << std::endl;
      }
    }
  }
  ss << "+-" << std::string(w1, '-')
     << "+-" << std::string(w2, '-')
     << "+-" << std::string(w3, '-')
     << "+" << std::endl;
  ss << std::endl;

  // Show all concepts, interfaces and functions.
  for (const auto& k : keys) {
    const size_t w1 = 20;
    const size_t w2 = 40;
    const size_t w = w1 + w2;
    const auto& m = *(modules_[k]);
    ss << std::string(w + 5, '=') << std::endl;
    ss << std::setw(w + 4)
       << (std::string("= module ") + m.name_with_api_version()) << "=" << std::endl;
    ss << std::string(w + 5, '=') << std::endl;

    // functions
    bool first_line = true;
    for (auto& function : m.functions()) {
      ss << "| " << std::setw(w1) << (first_line ? "functions" : "")
         << "| " << std::setw(w2) << function.first
         << "|" << std::endl;
      first_line = false;
    }
    if (!first_line) {
      ss << "+-" << std::string(w1, '-')
         << "+-" << std::string(w2, '-')
         << "+" << std::endl;
    }

    // concepts
    first_line = true;
    for (auto& concept : m.concepts()) {
      ss << "| " << std::setw(w1) << (first_line ? "concepts" : "")
         << "| " << std::setw(w2) << concept
         << "|" << std::endl;
      first_line = false;
    }
    if (!first_line) {
      ss << "+-" << std::string(w1, '-')
         << "+-" << std::string(w2, '-')
         << "+" << std::endl;
    }

    // implementations
    first_line = true;
    for (const auto& kv : m.implementations()) {
      const auto& implementation = kv.first;
      std::stringstream impl_info;
      impl_info << implementation << " (";

      // List implemented concepts.
      auto& concepts = kv.second.concepts;
      auto num_fields = concepts.size();
      for (size_t i = 0; i < num_fields; ++i) {
        auto concept = concepts[i];
        if (i > 0) {
          impl_info << ", ";
        }
        impl_info << concept.name;
      }
      impl_info << ")";

      ss << "| " << std::setw(w1) << (first_line ? "implementations" : "")
         << "| " << std::setw(w2) << impl_info.str()
         << "|" << std::endl;

      first_line = false;
    }
    if (!first_line) {
      ss << "+-" << std::string(w1, '-')
         << "+-" << std::string(w2, '-')
         << "+" << std::endl;
    }
    ss << std::endl;
  }

/*
  int msg_schemas_number = factory_->get_schemas_number();
  for (size_t i = 0; i < msg_schemas_number; ++i) {
    ss << factory_->get_schema_name(i) << std::endl;
  }
*/
  return ss.str();
}

std::unique_ptr<common::obj> registry::create(const data::msg& m) {
  auto msg_type = m.get_message_type();
  LOG(INFO) << "Creating object of type " << msg_type;

  // Route message to module factory.
  static const boost::regex obj_regex{"^([^\\.]+\\.v\\d*)\\.(.*)$"};
  boost::smatch what;
  if (boost::regex_search(msg_type, what, obj_regex)) {
    auto module_name = what[1].str();
    auto object_type = what[2].str();
    LOG(INFO) << "module: " << module_name;
    LOG(INFO) << "object type: " << object_type;
    if (modules_.count(module_name) == 0) {
      std::stringstream msg;
      msg << "No such module " << module_name;
      LOG(WARNING) << msg.str();
      throw std::invalid_argument(msg.str());
    }
    auto& mod = modules_.at(module_name);
    auto& impls = mod->implementations();
    if (impls.count(object_type) == 0) {
      std::stringstream msg;
      msg << "No factory found for object type " << object_type << " in module " << module_name;
      LOG(WARNING) << msg.str();
      throw std::invalid_argument(msg.str());
    }
    return impls.at(object_type).func(m);
  } else {
    // TODO(asen): Trhow an actual exception.
    throw "Invalid creation message!";
  }
}


}  // namespace script
}  // namespace core
}  // namespace automaton
