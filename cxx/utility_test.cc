#include <cxx/invoke.hpp>
#include <cxx/utility.hpp>

#include <gtest/gtest.h>

#include <utility>

TEST(cxx_utility, all_of_type) {
  EXPECT_TRUE((cxx::all_of_type<>::value));

  EXPECT_FALSE((cxx::all_of_type<false>::value));
  EXPECT_TRUE((cxx::all_of_type<true>::value));

  EXPECT_FALSE((cxx::all_of_type<false, false>::value));
  EXPECT_FALSE((cxx::all_of_type<false, true>::value));
  EXPECT_FALSE((cxx::all_of_type<true, false>::value));
  EXPECT_TRUE((cxx::all_of_type<true, true>::value));

  EXPECT_FALSE((cxx::all_of_type<false, false, false>::value));
  EXPECT_FALSE((cxx::all_of_type<false, false, true>::value));
  EXPECT_FALSE((cxx::all_of_type<false, true, false>::value));
  EXPECT_FALSE((cxx::all_of_type<false, true, true>::value));
  EXPECT_FALSE((cxx::all_of_type<true, false, false>::value));
  EXPECT_FALSE((cxx::all_of_type<true, false, true>::value));
  EXPECT_FALSE((cxx::all_of_type<true, true, false>::value));
  EXPECT_TRUE((cxx::all_of_type<true, true, true>::value));
}

TEST(cxx_utility, any_of_type) {
  EXPECT_FALSE((cxx::any_of_type<>::value));

  EXPECT_FALSE((cxx::any_of_type<false>::value));
  EXPECT_TRUE((cxx::any_of_type<true>::value));

  EXPECT_FALSE((cxx::any_of_type<false, false>::value));
  EXPECT_TRUE((cxx::any_of_type<false, true>::value));
  EXPECT_TRUE((cxx::any_of_type<true, false>::value));
  EXPECT_TRUE((cxx::any_of_type<true, true>::value));

  EXPECT_FALSE((cxx::any_of_type<false, false, false>::value));
  EXPECT_TRUE((cxx::any_of_type<false, false, true>::value));
  EXPECT_TRUE((cxx::any_of_type<false, true, false>::value));
  EXPECT_TRUE((cxx::any_of_type<false, true, true>::value));
  EXPECT_TRUE((cxx::any_of_type<true, false, false>::value));
  EXPECT_TRUE((cxx::any_of_type<true, false, true>::value));
  EXPECT_TRUE((cxx::any_of_type<true, true, false>::value));
  EXPECT_TRUE((cxx::any_of_type<true, true, true>::value));
}

TEST(cxx_utility, none_of_type) {
  EXPECT_TRUE((cxx::none_of_type<>::value));

  EXPECT_TRUE((cxx::none_of_type<false>::value));
  EXPECT_FALSE((cxx::none_of_type<true>::value));

  EXPECT_TRUE((cxx::none_of_type<false, false>::value));
  EXPECT_FALSE((cxx::none_of_type<false, true>::value));
  EXPECT_FALSE((cxx::none_of_type<true, false>::value));
  EXPECT_FALSE((cxx::none_of_type<true, true>::value));

  EXPECT_TRUE((cxx::none_of_type<false, false, false>::value));
  EXPECT_FALSE((cxx::none_of_type<false, false, true>::value));
  EXPECT_FALSE((cxx::none_of_type<false, true, false>::value));
  EXPECT_FALSE((cxx::none_of_type<false, true, true>::value));
  EXPECT_FALSE((cxx::none_of_type<true, false, false>::value));
  EXPECT_FALSE((cxx::none_of_type<true, false, true>::value));
  EXPECT_FALSE((cxx::none_of_type<true, true, false>::value));
  EXPECT_FALSE((cxx::none_of_type<true, true, true>::value));
}

namespace {
int i = 1;
int &ri = i;
const int ci = 2;
const int &rci = ci;

int fi(int i) { return i; }
int fri(int &ri) { return ri; }
int fmi(int &&mi) { return mi; }
int frci(const int &rci) { return rci; }

int frfi(int (&rfi)(int i), int i) { return rfi(i); }
int fpfi(int (*pfi)(int i), int i) { return pfi(i); }
}

namespace {
// Perfect forwarding
template <typename F, typename... Args> auto wrap(F &&f, Args &&... args) {
  return std::forward<F>(f)(std::forward<Args>(args)...);
}

template <typename F, typename... Args> auto wrap1(F &&f, Args &&... args) {
  return wrap(std::forward<F>(f), std::forward<Args>(args)...);
}

template <typename F, typename... Args> auto wrap2(F &&f, Args &&... args) {
  return wrap(wrap < F &&, Args && ... >, std::forward<F>(f),
              std::forward<Args>(args)...);
}

// Call with decay copy (such as e.g. async does)
template <typename F, typename... Args> auto call(F &&f, Args &&... args) {
  return std::decay_t<F>(f)(std::decay_t<Args>(args)...);
}

template <typename F, typename... Args> auto call1(F &&f, Args &&... args) {
  return call(std::decay_t<F>(f), std::decay_t<Args>(args)...);
}

template <typename F, typename... Args> auto call2(F &&f, Args &&... args) {
  return call(call<std::decay_t<F>, std::decay_t<Args>...>, std::decay_t<F>(f),
              std::decay_t<Args>(args)...);
}

// Combined
template <typename F, typename... Args> auto wrap_call(F &&f, Args &&... args) {
  return wrap(call < F &&, Args && ... >, std::forward<F>(f),
              std::forward<Args>(args)...);
}
}

TEST(cxx_utility, std_forward) {
  fi(1);
  fi(i);
  fi(ri);
  fi(ci);
  fi(rci);
  fri(i);
  fri(ri);
  fmi(1);
  frci(1);
  frci(i);
  frci(ri);
  frci(ci);
  frci(rci);
  frfi(fi, i);
  fpfi(fi, i);
  fpfi(&fi, i);

  wrap(fi, 1);
  wrap(fi, i);
  wrap(fi, ri);
  wrap(fi, ci);
  wrap(fi, rci);
  wrap(fri, i);
  wrap(fri, ri);
  wrap(fmi, 1);
  wrap(frci, 1);
  wrap(frci, i);
  wrap(frci, ri);
  wrap(frci, ci);
  wrap(frci, rci);
  wrap(frfi, fi, i);
  wrap(fpfi, fi, i);
  wrap(fpfi, &fi, i);

  wrap1(fi, 1);
  wrap1(fi, i);
  wrap1(fi, ri);
  wrap1(fi, ci);
  wrap1(fi, rci);
  wrap1(fri, i);
  wrap1(fri, ri);
  wrap1(fmi, 1);
  wrap1(frci, 1);
  wrap1(frci, i);
  wrap1(frci, ri);
  wrap1(frci, ci);
  wrap1(frci, rci);
  wrap1(frfi, fi, i);
  wrap1(fpfi, fi, i);
  wrap1(fpfi, &fi, i);

  wrap2(fi, 1);
  wrap2(fi, i);
  wrap2(fi, ri);
  wrap2(fi, ci);
  wrap2(fi, rci);
  wrap2(fri, i);
  wrap2(fri, ri);
  wrap2(fmi, 1);
  wrap2(frci, 1);
  wrap2(frci, i);
  wrap2(frci, ri);
  wrap2(frci, ci);
  wrap2(frci, rci);
  wrap2(frfi, fi, i);
  wrap2(fpfi, fi, i);
  wrap2(fpfi, &fi, i);

  call(fi, 1);
  call(fi, i);
  call(fi, ri);
  call(fi, ci);
  call(fi, rci);
  // call(fri, i);
  // call(fri, ri);
  call(fmi, 1);
  call(frci, 1);
  call(frci, i);
  call(frci, ri);
  call(frci, ci);
  call(frci, rci);
  // call(frfi, fi, i);
  call(fpfi, fi, i);
  call(fpfi, &fi, i);

  call1(fi, 1);
  call1(fi, i);
  call1(fi, ri);
  call1(fi, ci);
  call1(fi, rci);
  // call1(fri, i);
  // call1(fri, ri);
  call1(fmi, 1);
  call1(frci, 1);
  call1(frci, i);
  call1(frci, ri);
  call1(frci, ci);
  call1(frci, rci);
  // call1(frfi, fi, i);
  call1(fpfi, fi, i);
  call1(fpfi, &fi, i);

  call2(fi, 1);
  call2(fi, i);
  call2(fi, ri);
  call2(fi, ci);
  call2(fi, rci);
  // call2(fri, i);
  // call2(fri, ri);
  call2(fmi, 1);
  call2(frci, 1);
  call2(frci, i);
  call2(frci, ri);
  call2(frci, ci);
  call2(frci, rci);
  // call2(frfi, fi, i);
  call2(fpfi, fi, i);
  call2(fpfi, &fi, i);

  wrap_call(fi, 1);
  wrap_call(fi, i);
  wrap_call(fi, ri);
  wrap_call(fi, ci);
  wrap_call(fi, rci);
  // wrap_call(fri, i);
  // wrap_call(fri, ri);
  wrap_call(fmi, 1);
  wrap_call(frci, 1);
  wrap_call(frci, i);
  wrap_call(frci, ri);
  wrap_call(frci, ci);
  wrap_call(frci, rci);
  // wrap_call(frfi, fi, i);
  wrap_call(fpfi, fi, i);
  wrap_call(fpfi, &fi, i);
}

#if 0
TEST(cxx_utility, is_any_function) {
  EXPECT_FALSE((cxx::is_any_function<>::value));
  EXPECT_FALSE((cxx::is_any_function<int>::value));
  EXPECT_FALSE((cxx::is_any_function<int, char>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(int)>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(int (*)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(int(&)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(int(&&)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&)(int)>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&)(int (*)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&)(int(&)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&)(int(&&)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&&)(int)>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&&)(int (*)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&&)(int(&)(int))>::value));
  EXPECT_TRUE((cxx::is_any_function<int, char, int(&&)(int(&&)(int))>::value));
  EXPECT_FALSE((cxx::is_any_function<int, char, int (*)(int)>::value));
  EXPECT_FALSE((cxx::is_any_function<int, char, int (*)(int (*)(int))>::value));
  EXPECT_FALSE((cxx::is_any_function<int, char, int (*)(int(&)(int))>::value));
  EXPECT_FALSE((cxx::is_any_function<int, char, int (*)(int(&&)(int))>::value));
}

TEST(cxx_utility, has_any_function_arguments) {
  EXPECT_FALSE((cxx::has_any_function_arguments<>::value));
  EXPECT_FALSE((cxx::has_any_function_arguments<int>::value));
  EXPECT_FALSE((cxx::has_any_function_arguments<int, char>::value));
  EXPECT_FALSE(
      (cxx::has_any_function_arguments<int, char, int (*)(int)>::value));
  EXPECT_TRUE((
      cxx::has_any_function_arguments<int, char, int (*)(int(&)(int))>::value));
  EXPECT_FALSE((cxx::has_any_function_arguments<int, char, int(int)>::value));
  EXPECT_FALSE(
      (cxx::has_any_function_arguments<int, char, int(int (*)(int))>::value));
  EXPECT_TRUE(
      (cxx::has_any_function_arguments<int, char, int(int(&)(int))>::value));
  EXPECT_TRUE(
      (cxx::has_any_function_arguments<int, char, int(int(&&)(int))>::value));
  EXPECT_FALSE(
      (cxx::has_any_function_arguments<int, char, int(&)(int)>::value));
  EXPECT_FALSE((
      cxx::has_any_function_arguments<int, char, int(&)(int (*)(int))>::value));
  EXPECT_TRUE(
      (cxx::has_any_function_arguments<int, char, int(&)(int(&)(int))>::value));
  EXPECT_TRUE((
      cxx::has_any_function_arguments<int, char, int(&)(int(&&)(int))>::value));
  EXPECT_FALSE(
      (cxx::has_any_function_arguments<int, char, int(&&)(int)>::value));
  EXPECT_FALSE((cxx::has_any_function_arguments<int, char,
                                                int(&&)(int (*)(int))>::value));
  EXPECT_TRUE((
      cxx::has_any_function_arguments<int, char, int(&&)(int(&)(int))>::value));
  EXPECT_TRUE((cxx::has_any_function_arguments<int, char,
                                               int(&&)(int(&&)(int))>::value));
}
#endif
