#include "automaton/core/network/connection.h"

namespace automaton {
namespace core {
namespace network {

connection::connection(connection_id id_, std::shared_ptr<connection::connection_handler> handler_):
    handler(handler_), id(id_) {}

std::shared_ptr<connection> connection::create(const std::string& type, connection_id id, const std::string& address,
    std::shared_ptr<connection::connection_handler> handler) {
  auto it = connection_factory.find(type);
  if (it != connection_factory.end()) {
    return it->second(id, address, handler);
  }
  return nullptr;
}

connection_id connection::get_id() {
  return id;
}

void connection::register_connection_type(const std::string& type, factory_function func) {
  connection_factory[type] = func;
}

std::map<std::string, connection::factory_function>
  connection::connection_factory;

}  // namespace network
}  // namespace core
}  // namespace automaton
