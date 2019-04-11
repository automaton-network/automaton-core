#include "automaton/core/network/acceptor.h"

namespace automaton {
namespace core {
namespace network {

acceptor::acceptor(acceptor_id id_, std::shared_ptr<acceptor::acceptor_handler> handler_):handler(handler_), id(id_) {}

acceptor_id acceptor::get_id() {
  return id;
}

std::shared_ptr<acceptor> acceptor::create(const std::string& type, acceptor_id id, const std::string& address,
    std::shared_ptr<acceptor::acceptor_handler> handler,
    std::shared_ptr<connection::connection_handler> connections_handler) {
  auto it = acceptor_factory.find(type);
  if (it == acceptor_factory.end()) {
    return NULL;
  } else {
    return it -> second(id, address, handler, connections_handler);
  }
}
void acceptor::register_acceptor_type(
    const std::string& type, factory_function func) {
  acceptor_factory[type] = func;
}

std::map<std::string, acceptor::factory_function>
    acceptor::acceptor_factory;

}  // namespace network
}  // namespace core
}  // namespace automaton
