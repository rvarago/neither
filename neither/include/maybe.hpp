#ifndef NEITHER_MAYBE_HPP
#define NEITHER_MAYBE_HPP

#include <memory>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <neither/traits.hpp>
#include <iterator>

namespace neither {

template <class T> struct Maybe;

template <> struct Maybe<void> {};

template <class T> struct Maybe {

  using size_type = std::size_t;

  union {
    T value;
  };

  bool const hasValue;

  constexpr Maybe() : hasValue{false} {}

  constexpr Maybe(T const& value) :  value{value}, hasValue{true} {}
  constexpr Maybe(T&& value) :  value{std::move(value)}, hasValue{true} {}

  constexpr Maybe(Maybe<void>) : hasValue{false} {}

  constexpr Maybe(Maybe<T> const &o) : hasValue{o.hasValue} {
    if (o.hasValue) {
      new (&value)T(o.value);
    }
  }

  ~Maybe() {
    if (hasValue) {
      value.~T();
    }
  }

  constexpr T get(T defaultValue) {
    return hasValue ? value : defaultValue;
  }

  constexpr T unsafeGet() {
    assert(hasValue && "unsafeGet must not be called on an empty Maybe");
    return value;
  }

  constexpr size_type size() const noexcept { return hasValue ? 1: 0; }
  
  constexpr bool empty() const noexcept { return !hasValue; }
  
  template<class F>
    constexpr auto map(F const &f) const&
    -> Maybe<decltype(f(isCopyable(value)))> {
    using ReturnType = decltype(f(value));
    if (!hasValue) {
      return Maybe<ReturnType>();
    }
    return Maybe<ReturnType>(f(value));
  }


  template<class F>
    auto map(F const& f)&&
    -> Maybe<decltype(f(std::move(value)))> {
    using ReturnType = decltype(f(std::move(value)));
    if (!hasValue) {
      return Maybe<ReturnType>();
    }
    return Maybe<ReturnType>(f(std::move(value)));
  }

  template <class F>
  constexpr auto flatMap(F const& f) const&
    -> decltype(ensureMaybe(f(value))) {
    using ReturnType = decltype(f(value));
    if (!hasValue) {
      return ReturnType();
    }

    return f(value);
  }

  template <class F>
  constexpr auto flatMap(F const& f)&&
    -> decltype(ensureMaybe(f(std::move(value)))) {
    using ReturnType = decltype(f(std::move(value)));
    if (!hasValue) {
      return ReturnType();
    }

    return f(std::move(value));
  }

  constexpr operator bool() const { return hasValue; }


  constexpr iterator begin() {
      if (empty()) {
          return end();
      }
      return iterator<T>{*this};
  }

  constexpr iterator end() {
      return iterator<T>{};
  }

  constexpr const_iterator begin() const {
      if (empty()) {
          return end()
      }
      return iterator<const T>{*this};
  }

  constexpr const_iterator end() const {
      return iterator<T>{};
  }
};

template <typename T>
auto maybe(T value) -> Maybe<T> { return {value}; }

template <typename T = void>
auto maybe() -> Maybe<T> { return {}; }

namespace {

  bool equal(Maybe<void> const&, Maybe<void> const&) {
    return true;
  }

  template <typename T>
  bool equal(Maybe<T> const &a, Maybe<T> const &b) {
    if (a.hasValue) {
      return b.hasValue && a.value == b.value;
    }
    return !b.hasValue;
  }
}

template <typename T>
bool operator == (Maybe<T> const& a, Maybe<T> const& b) {
  return equal(a, b);
}

template <typename T>
bool operator != (Maybe<T> const& a, Maybe<T> const& b) {
  return !(a == b);
}

static const auto none = maybe();

namespace detail {

  template <typename T>
  class maybe_iterator {
  public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = std::remove_cv_t<T>;
      using difference_type = std::ptrdiff_t;
      using pointer = value_type*;
      using reference = value_type&;

      constexpr maybe_iterator() = default;

      constexpr explicit maybe_iterator(Maybe<T> const& container) :
        underlying_container{&container} {}

      constexpr const reference operator++() {
          visited = true;
          return underlying_container->unsafeGet();
      }

      constexpr const reference operator++(int) {
          return ++this;
      }

      constexpr const reference operator*() const
      {
          return underlying_container->unsafeGet();
      }

      constexpr const reference operator->() const
      {
          return underlying_container->unsafeGet();
      }

  private:
      Maybe<T>* underlying_container{nullptr};
      bool visited{false};
  };

  template <typename T1, typename T2>
  constexpr bool operator==(maybe_iterator<T1> const& lhs, maybe_iterator<T2> const& rhs) {
      return lhs.underlying_container == rhs.underlying_container && lhs.visited == rhs.visited;
  }

  template <typename T1, typename T2>
  constexpr bool operator!=(maybe_iterator<T1> const& lhs, maybe_iterator<T2> const& rhs) {
      return !(lhs == rhs);
  }

}


}

#endif
