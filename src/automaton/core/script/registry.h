#ifndef AUTOMATON_CORE_SCRIPT_REGISTRY_H_
#define AUTOMATON_CORE_SCRIPT_REGISTRY_H_

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/regex.hpp>

#include "automaton/core/data/factory.h"
#include "automaton/core/data/msg.h"
#include "automaton/core/data/schema.h"
#include "automaton/core/io/io.h"

namespace automaton {
namespace core {
namespace script {

/**
  Module interface definition base class.
*/
class module {
 public:
  typedef std::unique_ptr<common::obj> (*constructor_function)(const data::msg& m);
  typedef common::status (*module_static_function)(const data::msg& m, data::msg* output);

  struct static_function_info {
    std::string name;
    module_static_function func;
    uint32_t input_schema_id;
    uint32_t output_schema_id;
  };

  struct method_info {
    std::string name;
    uint32_t input_schema_id;
    uint32_t output_schema_id;
  };

  struct concept_info {
    std::string name;
    std::vector<method_info> methods;
  };

  struct implementation_info {
    std::string name;
    constructor_function func;
    uint32_t constructor_schema_id;
    std::vector<concept_info> concepts;
  };

  virtual const std::string name() const { return name_; }

  virtual const std::string name_with_api_version() const {
    return name_with_api_version(name_, api_version_);
  }

  virtual const std::string full_version() const { return version_; }

  virtual const uint32_t api_version() const { return api_version_; }

  virtual const uint32_t minor_version() const { return minor_version_; }

  virtual const uint32_t patch_version() const { return patch_version_; }

  virtual const std::string extra_version() const { return extra_version_; }

  virtual data::schema* schema() const = 0;

  virtual void bind_schemas();

  virtual const std::vector<std::string> dependencies() const {
    return dependencies_;
  }

  virtual const std::unordered_map<std::string, static_function_info> functions() const {
    return functions_;
  }

  virtual const std::vector<std::string> concepts() const {
    return concepts_;
  }

  virtual const std::unordered_map<std::string, implementation_info> implementations() const {
    return implementations_;
  }

  // virtual void process(const data::msg& input, data::msg* output) = 0;

  void check_implementation(const std::string implementation);

 protected:
  module(const std::string name, const std::string version) : name_(name), version_(version) {
    static const boost::regex version_regex{"^(\\d+)(\\.\\d+)(\\.\\d+)(\\..*)$"};
    boost::smatch what;
    if (boost::regex_search(version_, what, version_regex)) {
      api_version_ = std::atoi(what[1].str().c_str());
      minor_version_ = std::atoi(what[2].str().c_str());
      patch_version_ = std::atoi(what[3].str().c_str());
      extra_version_ = what[4].str();
    } else {
      // TODO(asen): Trhow an actual exception.
      std::stringstream msg;
      msg << "Invalid version";
      LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::invalid_argument(msg.str());
    }
  }

  void add_dependency(const std::string dependency, uint32_t api_version) {
    dependencies_.push_back(name_with_api_version(dependency, api_version));
  }

  void add_concept(const std::string concept) {
    concepts_.push_back(concept);
  }

  void add_function(const std::string function_name, module_static_function func);

  void add_implementation(const std::string implementation,
                          const std::vector<std::string> concepts,
                          constructor_function f);

 private:
  static const std::string name_with_api_version(const std::string name, uint32_t api_version) {
    return name + ".v" + std::to_string(api_version);
  }
  std::string name_;
  std::string version_;
  uint32_t api_version_;
  uint32_t minor_version_;
  uint32_t patch_version_;
  std::string extra_version_;
  std::vector<std::string> dependencies_;
  std::vector<std::string> concepts_;
  std::unordered_map<std::string, implementation_info> implementations_;
  std::unordered_map<std::string, static_function_info> functions_;
};

/**
  Registry for scriptable modules.
*/
class registry {
 public:
  registry(registry&) = delete;
  registry(const registry&) = delete;

  /**
    Binds module M to the script::registry.
  */
  template<typename M>
  void import() {
    M& m = M::instance();
    LOG(INFO) << "Importing module " << m.name() << " " << &(M::instance());
    if (modules_.count(m.name_with_api_version()) > 0) {
      std::stringstream msg;
      msg << "Module [" << m.name_with_api_version() << "] has already been imported!";
      LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
      throw std::runtime_error(msg.str());
      // throw "Already registered!";
    }
    modules_[m.name_with_api_version()] = &m;
    std::string json;
    m.schema()->to_json(&json);

    // Ensure dependencies are already imported.
    for (const auto& dep : m.dependencies()) {
      if (modules_.count(dep) == 0) {
        std::stringstream msg;
        msg << m.name_with_api_version() << " depends on " << dep <<
            " which hasn't been imported yet!";
        LOG(ERROR) << msg.str() << '\n' << el::base::debug::StackTrace();
        throw std::runtime_error(msg.str());
        // throw "dependency issue!";
      }
      const auto& dep_module = modules_[dep];
      m.schema()->add_dependency(dep_module->name_with_api_version());
    }
    m.schema()->to_json(&json);

    // Import schema.
    LOG(INFO) << "Importing schema " << m.name_with_api_version();
    factory_->import_schema(m.schema(), m.name_with_api_version(), m.name_with_api_version());
    m.bind_schemas();

    for (auto& impl : m.implementations()) {
      m.check_implementation(impl.first);
    }
  }

  /**
    Dumps information about all registered modules, functions, classes and schemas into a string.
  */
  std::string to_string();

  /**
    Gets reference to the registry instance.

    Creates the instance when called for the first time.
  */
  static registry& instance() {
    static registry* inst = nullptr;
    if (inst == nullptr) {
      LOG(INFO) << "Creating registry.";
      inst = new registry();
    }
    return *inst;
  }

  data::factory& get_factory() { return *factory_.get() ; }
  std::unique_ptr<common::obj> create(const data::msg& m);

  void process(const data::msg& request, data::msg* response);

  std::vector<std::string> module_names() {
    std::vector<std::string> names;
    for (const auto& m : modules_) {
      names.push_back(m.first);
    }
    return names;
  }

  module* get_module(std::string name) {
    if (modules_.count(name) == 0) {
      return nullptr;
    }
    return modules_[name];
  }

 private:
  registry();

  std::unordered_map<std::string, module*> modules_;
  std::unique_ptr<data::factory> factory_;
  // std::unordered_map<uint64_t, common::obj> objects_;
};

}  // namespace script
}  // namespace core
}  // namespace automaton

#endif  // AUTOMATON_CORE_SCRIPT_REGISTRY_H_
