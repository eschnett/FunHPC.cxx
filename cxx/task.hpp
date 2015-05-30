#ifndef CXX_TASK_HPP
#define CXX_TASK_HPP

#include <cxx/apply.hpp>
#include <cxx/serialize.hpp>

#include <cereal/access.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/tuple.hpp>

#include <memory>
#include <tuple>
#include <typeinfo>
#include <utility>

namespace cxx {

// helpers for cereal //////////////////////////////////////////////////////////

namespace detail {
template <typename T>
using cereal_register_t = cereal::detail::bind_to_archives<T> const &;

template <typename T> cereal_register_t<T> cereal_register_init() {
  return cereal::detail::StaticObject<
             cereal::detail::bind_to_archives<T>>::getInstance().bind();
}
}

// task ////////////////////////////////////////////////////////////////////////

// A task is similar to std::bind, except that it does not support
// placeholders. It is also similar to unique_function, except that
// the argument list is always empty. It is also similar to
// packaged_task, except that the return value is returned and not
// stored in a promise. In addition to the above, it explicitly
// converts the return type to the requested type R. (R may be void).

namespace detail {
template <typename R> class abstract_task {
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {}

public:
  virtual ~abstract_task() {}
  virtual R operator()() = 0;
};

template <typename R, typename F, typename... Args>
class concrete_task final : public abstract_task<R> {
  static_assert(std::is_same<F, std::decay_t<F>>::value, "");
  static_assert(std::is_same<std::tuple<Args...>,
                             std::tuple<std::decay_t<Args>...>>::value,
                "");
  F f;
  std::tuple<Args...> args;
#ifndef NDEBUG
  bool did_call = false;
#endif

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    assert(!did_call);
    ar(cereal::base_class<abstract_task<R>>(this), f, args);
  }
  static cereal_register_t<concrete_task> cereal_register;

public:
  concrete_task() {} // only for serialization
  template <typename F1, typename... Args1>
  concrete_task(F1 &&f, Args1 &&... args)
      : f(std::forward<F1>(f)),
        args(std::make_tuple(std::forward<Args1>(args)...)) {}
  virtual ~concrete_task() {}
  virtual R operator()() {
    assert(!did_call);
#ifndef NDEBUG
    did_call = true;
#endif
    return R(cxx::apply(std::move(f), std::move(args)));
  }
  static void register_type() { (void)cereal_register; }
};
template <typename R, typename F, typename... Args>
cereal_register_t<concrete_task<R, F, Args...>>
    concrete_task<R, F, Args...>::cereal_register =
        cereal_register_init<concrete_task<R, F, Args...>>();
}

template <typename R> class task {
  std::unique_ptr<detail::abstract_task<R>> ptask;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(ptask); }

public:
  task() noexcept {}
  task(task &&other) noexcept : ptask() { swap(other); }
  task(const task &other) = delete;
  template <typename F, typename... Args>
  task(F &&f, Args &&... args)
      : ptask(std::make_unique<
            detail::concrete_task<R, std::decay_t<F>, std::decay_t<Args>...>>(
            std::forward<F>(f), std::forward<Args>(args)...)) {}
  task &operator=(task &&other) {
    ptask = {};
    swap(other);
    return *this;
  }
  task &operator=(const task &other) = delete;
  void swap(task &other) {
    using std::swap;
    swap(ptask, other.ptask);
  }
  R operator()() { return (*ptask)(); }
  template <typename F, typename... Args> static void register_type() {
    detail::concrete_task<R, F, Args...>::register_type();
  }
};
template <typename R> void swap(task<R> &lhs, task<R> &rhs) { lhs.swap(rhs); }
}

namespace cereal {
namespace detail {
template <typename R> struct binding_name<::cxx::detail::abstract_task<R>> {
  static constexpr char const *name() {
    return typeid(::cxx::detail::abstract_task<R>).name();
  }
};

template <typename R, typename F, typename... Args>
struct binding_name<::cxx::detail::concrete_task<R, F, Args...>> {
  static constexpr char const *name() {
    return typeid(::cxx::detail::concrete_task<R, F, Args...>).name();
  }
};
}
}

#define CXX_TASK_HPP_DONE
#endif // #ifndef CXX_TASK_HPP
#ifndef CXX_TASK_HPP_DONE
#error "Cyclic include dependency"
#endif
