#include "memory"
#include <boost/di.hpp>
#include "gtest/gtest.h"

namespace di = boost::di;

// FORWARD BINDINGS
namespace forward_bindings {

class interface;
class implementation;

auto configuration = [] {
  // <<binding using fwd declarations, no checking whether types are related
  return di::make_injector(di::bind<interface>().to<implementation>());
};

/*<<binding using fwd declarations, no checking whether types are related*/
class interface {
 public:
  virtual ~interface() noexcept = default;
  virtual void dummy() = 0;
};

class implementation : public interface {
 public:
  void dummy() override {}
};

TEST(test_di, forward_bindings) {
  /*<<make injector configuration>>*/
  auto injector = configuration();
  ASSERT_TRUE(dynamic_cast<implementation*>(injector.create<std::unique_ptr<interface>>().get()));
}

}  // namespace forward_bindings

// DYNAMIC BINDINGS
namespace dynamic_bindings {
enum eid { e1 = 1, e2 = 2 };
struct interface {
  virtual ~interface() noexcept = default;
};
struct implementation1 : interface {};
struct implementation2 : interface {};

auto dynamic_bindings = [](const eid& id) {
  return di::make_injector(
      /*<<bind `interface` to lazy lambda expression>>*/
      di::bind<interface>().to([&](const auto& injector) -> std::shared_ptr<interface> {
        switch (id) {
          default:
            return nullptr;
          case e1:
            return injector.template create<std::shared_ptr<implementation1>>();
          case e2:
            return injector.template create<std::shared_ptr<implementation2>>();
        }

        return nullptr;
      }));
};

TEST(test_di, dynamic_bindings) {
  auto id = e1;

  /*<<create interface with `id = e1`>>*/
  auto injector = di::make_injector(dynamic_bindings(id));
  ASSERT_TRUE(dynamic_cast<implementation1*>(injector.create<std::shared_ptr<interface>>().get()));

  id = e2;
  /*<<create interface with `id = e2`>>*/
  ASSERT_TRUE(dynamic_cast<implementation2*>(injector.create<std::shared_ptr<interface>>().get()));
  (void)id;
}

}  // namespace dynamic_bindings

// ANNOTATIONS
namespace annotations {

auto int_1 = [] {};
struct int_2_t {
} int_2;

struct annotations1 {
  /*<<Constructor with named parameters of the same `int` type>>*/
  BOOST_DI_INJECT(
      annotations1,
      (named = int_1) int i1,
      (named = int_2) int i2,
      int i3) : i1(i1), i2(i2), i3(i3) {}

  int i1 = 0;
  int i2 = 0;
  int i3 = 0;
};

TEST(boost_di, annotations) {
  {
    /*<<make injector and bind named parameters>>*/
    auto injector = di::make_injector(
      di::bind<int>().named(int_1).to(42)
    , di::bind<int>().named(int_2).to(87)
    , di::bind<int>().to(123));

    /*<<create `annotations`>>*/
    auto a1 = injector.create<annotations1>();
    ASSERT_EQ(a1.i1, 42);
    ASSERT_EQ(a1.i2, 87);
    ASSERT_EQ(a1.i3, 123);
  }
}

}  // namespace annotations
