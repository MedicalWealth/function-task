#include "function.h"

#include <gtest/gtest.h>

TEST(function_test, default_ctor) {
  function<void()> x;
  function<void(int, int, int)> y;
  function<double && (float&, const int&, int)> z;
}

TEST(function_test, empty_convertion_to_bool) {
  function<void()> x;
  EXPECT_FALSE(static_cast<bool>(x));
  EXPECT_THROW(x(), bad_function_call);
}

TEST(function_test, empty_call) {
  function<void()> x;
  EXPECT_THROW(x(), bad_function_call);
}

TEST(function_test, empty_copy_move) {
  function<void()> x;

  function<void()> y = x;
  EXPECT_FALSE(static_cast<bool>(y));

  function<void()> z = std::move(x);
  EXPECT_FALSE(static_cast<bool>(z));

  z = y;
  EXPECT_FALSE(static_cast<bool>(z));

  y = std::move(z);
  EXPECT_FALSE(static_cast<bool>(y));
}

TEST(function_test, ctor_func) {
  function<int()> f = [] { return 42; };
  EXPECT_EQ(42, f());
}

TEST(function_test, ctor_copy) {
  function<int()> f = [] { return 42; };
  function<int()> g = f;
  EXPECT_EQ(42, f());
  EXPECT_EQ(42, g());
}

namespace {

template <typename T, typename F>
bool is_small(const F& f) {
  return std::less_equal<>{}(static_cast<const void*>(&f), static_cast<const void*>(f.template target<T>())) &&
         std::less<>{}(static_cast<const void*>(f.template target<T>()), static_cast<const void*>(&f + 1));
}

struct small_func {
  explicit small_func(int value) noexcept : value(value) {}

  small_func(const small_func&) = default;
  small_func& operator=(const small_func&) = default;

  small_func(small_func&& other) noexcept {
    *this = std::move(other);
  }

  small_func& operator=(small_func&& other) noexcept {
    value = std::exchange(other.value, 0);
    return *this;
  }

  int operator()() const {
    return value;
  }

  int get_value() const {
    return value;
  }

private:
  int value;
};

} // namespace

TEST(function_test, empty_target) {
  function<int()> f;
  EXPECT_EQ(nullptr, f.target<small_func>());
  EXPECT_EQ(nullptr, std::as_const(f).target<small_func>());
}

TEST(function_test, small_func) {
  function<int()> f = small_func(42);
  EXPECT_EQ(42, f());
  EXPECT_TRUE(is_small<small_func>(f));
}

TEST(function_test, small_func_copy_ctor) {
  function<int()> f = small_func(42);
  function<int()> g = f;
  EXPECT_EQ(42, f());
  EXPECT_EQ(42, g());
}

TEST(function_test, small_func_move_ctor) {
  function<int()> f = small_func(42);
  function<int()> g = std::move(f);
  EXPECT_EQ(42, g());
}

TEST(function_test, small_func_assignment_operator) {
  function<int()> f = small_func(42);
  function<int()> g;
  g = f;
  EXPECT_EQ(42, f());
  EXPECT_EQ(42, g());
}

TEST(function_test, small_func_assignment_operator_move) {
  function<int()> f = small_func(42);
  function<int()> g;
  g = std::move(f);
  EXPECT_EQ(42, g());
}

TEST(function_test, small_func_assignment_operator_self) {
  function<int()> f = small_func(42);
  f = f;
  EXPECT_EQ(42, f());
}

TEST(function_test, small_func_assignment_operator_move_self) {
  function<int()> f = small_func(42);
  f = std::move(f);
  EXPECT_EQ(42, f());
}

namespace {

struct small_func_with_pointer {
  explicit small_func_with_pointer() : pointer(this) {}

  void swap(small_func_with_pointer& other) noexcept {
    std::swap(pointer, other.pointer);
  }

  small_func_with_pointer(const small_func_with_pointer&) noexcept : pointer(this) {}

  small_func_with_pointer& operator=(const small_func_with_pointer& other) noexcept {
    if (this != &other) {
      small_func_with_pointer(other).swap(*this);
    }
    return *this;
  }

  small_func_with_pointer(small_func_with_pointer&&) noexcept : pointer(this) {}

  small_func_with_pointer& operator=(small_func_with_pointer&& other) noexcept {
    if (this != &other) {
      small_func_with_pointer(std::move(other)).swap(*this);
    }
    return *this;
  }

  bool operator()() const {
    return pointer == this;
  }

private:
  small_func_with_pointer* pointer;
};

} // namespace

TEST(function_test, small_func_with_pointer) {
  function<int()> f = small_func_with_pointer();
  EXPECT_TRUE(is_small<small_func_with_pointer>(f));
}

TEST(function_test, small_func_with_pointer_copy_ctor) {
  function<int()> f = small_func_with_pointer();
  function<int()> g(f);
  EXPECT_TRUE(f());
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_with_pointer_assignment_operator_copy) {
  function<int()> f = small_func_with_pointer();
  function<int()> g = small_func_with_pointer();
  f = g;
  EXPECT_TRUE(f());
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_with_pointer_move_ctor) {
  function<int()> f = small_func_with_pointer();
  function<int()> g(std::move(f));
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_with_pointer_assignment_operator_move) {
  function<int()> f = small_func_with_pointer();
  function<int()> g = small_func_with_pointer();
  g = std::move(f);
  EXPECT_TRUE(g());
}

TEST(function_test, small_func_target) {
  function<int()> f = small_func(42);
  EXPECT_EQ(42, f.target<small_func>()->get_value());
  EXPECT_EQ(42, std::as_const(f).target<small_func>()->get_value());
}

namespace {

struct large_func {
  large_func(int value) noexcept : that(this), value(value) {
    ++n_instances;
  }

  large_func(const large_func& other) noexcept : that(this), value(other.value) {
    ++n_instances;
  }

  large_func& operator=(const large_func& rhs) noexcept {
    value = rhs.value;
    return *this;
  }

  ~large_func() {
    assert(this == that);
    --n_instances;
  }

  int operator()() const noexcept {
    assert(this == that);
    return value;
  }

  static void assert_no_instances() {
    assert(n_instances == 0);
  }

  int get_value() const {
    return value;
  }

private:
  large_func* that;
  int value;
  int payload[1000];

  inline static size_t n_instances = 0;
};

} // namespace

TEST(function_test, large_func) {
  {
    function<int()> f = large_func(42);
    EXPECT_EQ(42, f());
  }
  large_func::assert_no_instances();
}

TEST(function_test, large_func_copy_ctor) {
  function<int()> f = large_func(42);
  function<int()> g = f;
  EXPECT_EQ(42, f());
  EXPECT_EQ(42, g());
}

TEST(function_test, large_func_move_ctor) {
  function<int()> f = large_func(42);
  function<int()> g = std::move(f);
  EXPECT_EQ(42, g());
}

TEST(function_test, large_func_assignment_operator) {
  function<int()> f = large_func(42);
  function<int()> g;
  g = f;
  EXPECT_EQ(42, g());
}

TEST(function_test, large_func_assignment_operator_move) {
  function<int()> f = large_func(42);
  function<int()> g;
  g = std::move(f);
  EXPECT_EQ(42, g());
}

TEST(function_test, large_func_assignment_operator_self) {
  function<int()> f = large_func(42);
  f = f;
  EXPECT_EQ(42, f());
}

TEST(function_test, large_func_assignment_operator_move_self) {
  function<int()> f = large_func(42);
  f = std::move(f);
  EXPECT_EQ(42, f());
}

TEST(function_test, large_func_target) {
  function<int()> f = large_func(42);
  EXPECT_EQ(42, f.target<large_func>()->get_value());
  EXPECT_EQ(42, std::as_const(f).target<large_func>()->get_value());
}

namespace {

struct throwing_copy {
  struct exception final : std::exception {
    using std::exception::exception;
  };

  throwing_copy() = default;

  int operator()() const {
    return 43;
  }

  throwing_copy(const throwing_copy&) {
    throw exception();
  }

  throwing_copy(throwing_copy&&) noexcept {}

  throwing_copy& operator=(const throwing_copy&) = delete;
  throwing_copy& operator=(throwing_copy&&) = delete;
};

} // namespace

TEST(function_test, small_func_throwing_copy) {
  function<int()> f = small_func(42);
  function<int()> g = throwing_copy();

  EXPECT_TRUE(is_small<throwing_copy>(g));

  EXPECT_THROW(f = g, throwing_copy::exception);
  EXPECT_EQ(42, f());

  EXPECT_NO_THROW(f = std::move(g));
  EXPECT_EQ(43, f());
}

TEST(function_test, large_func_throwing_copy) {
  function<int()> f = large_func(42);
  function<int()> g = throwing_copy();

  EXPECT_TRUE(is_small<throwing_copy>(g));

  EXPECT_THROW(f = g, throwing_copy::exception);
  EXPECT_EQ(42, f());

  EXPECT_NO_THROW(f = std::move(g));
  EXPECT_EQ(43, f());
}

namespace {

struct throwing_move {
  struct exception final : std::exception {
    using std::exception::exception;
  };

  throwing_move() = default;

  int operator()() const {
    return 43;
  }

  throwing_move(const throwing_move&) noexcept {}

  throwing_move(throwing_move&&) {
    throw exception();
  }

  throwing_move& operator=(const throwing_move&) = delete;
  throwing_move& operator=(throwing_move&&) = delete;
};

} // namespace

TEST(function_test, throwing_move) {
  function<int()> f = small_func(42);
  function<int()> g = throwing_copy();

  EXPECT_NO_THROW(f = std::move(g));
  EXPECT_EQ(43, f());
}

TEST(function_test, arguments) {
  function<int(int, int)> f = [](int a, int b) { return a + b; };

  EXPECT_EQ(42, f(40, 2));
}

TEST(function_test, arguments_ref) {
  int x = 42;
  function<int&(int&)> f = [](int& a) -> int& { return a; };

  EXPECT_EQ(&x, &f(x));
}

TEST(function_test, arguments_cref) {
  const int x = 42;
  function<const int&(const int&)> f = [](const int& a) -> const int& { return a; };

  EXPECT_EQ(&x, &f(x));
}

namespace {

struct non_copyable {
  non_copyable() {}

  non_copyable(const non_copyable&) = delete;
  non_copyable(non_copyable&&) = default;
};

} // namespace

TEST(function_test, argument_by_value) {
  function<non_copyable(non_copyable)> f = [](non_copyable a) { return std::move(a); };
  non_copyable a = f(non_copyable());
}

TEST(function_test, argument_by_value_large) {
  int big_array[1000];
  function<non_copyable(non_copyable)> f = [big_array](non_copyable a) { return std::move(a); };
  non_copyable a = f(non_copyable());
}

TEST(function_test, recursive_test) {
  function<int(int)> fib = [&fib](int n) -> int {
    switch (n) {
    case 0:
      return 0;
    case 1:
      return 1;
    default:
      return fib(n - 1) + fib(n - 2);
    }
  };
  EXPECT_EQ(55, fib(10));
}

TEST(function_test, target) {
  struct foo {
    void operator()() const {}
  };

  struct bar {
    void operator()() const {}
  };

  function<void()> f = foo();
  EXPECT_NE(nullptr, f.target<foo>());
  EXPECT_EQ(nullptr, f.target<bar>());
  EXPECT_NE(nullptr, std::as_const(f).target<foo>());
  EXPECT_EQ(nullptr, std::as_const(f).target<bar>());
  f = bar();
  EXPECT_EQ(nullptr, f.target<foo>());
  EXPECT_NE(nullptr, f.target<bar>());
  EXPECT_EQ(nullptr, std::as_const(f).target<foo>());
  EXPECT_NE(nullptr, std::as_const(f).target<bar>());
}
