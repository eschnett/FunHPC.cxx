#include <cxx/funobj.hpp>

#include <gtest/gtest.h>

namespace {

// regular function
int fun(int x) { return x + 1; }

// lambda expression
const auto lam = [](int x) { return x + 1; };

// function object
struct fobj {
  int operator()(int x) const { return x + 1; }
};

struct s {
  // member function
  int memfun(int x) const { return x + 1; }

  // member object
  int memobj;
};
}

TEST(cxx_funobj, funobj) {
  auto fun1 = CXX_FUNOBJ(fun);
  EXPECT_EQ(2, fun1(1));

  // Dont' know how to efficiently wrap lambdas yet
  auto lam1 = lam;
  EXPECT_EQ(2, lam1(1));

  // Function objects don't need wrapping
  auto fobj1 = fobj();
  EXPECT_EQ(2, fobj1(1));

  auto memfun1 = CXX_FUNOBJ(&s::memfun);
  EXPECT_EQ(2, memfun1(s(), 1));

  s s1;
  auto memobj1 = CXX_FUNOBJ(&s::memobj);
  memobj1(s1) = 1;
  EXPECT_EQ(1, memobj1(s1));
}

namespace {
const int cobj = 4;
int obj = 5;
}

TEST(cxx_funobj, obj) {
  auto cobj0 = cxx::detail::obj_impl<int, cobj>();
  auto cobj1 = CXX_OBJ(cobj);
  EXPECT_EQ(4, cobj0());
  EXPECT_EQ(4, cobj1());

  auto cobj_ptr0 = cxx::detail::obj_impl<const int *, &cobj>();
  auto cobj_ptr1 = CXX_OBJ(&cobj);
  EXPECT_EQ(4, *cobj_ptr0());
  EXPECT_EQ(4, *cobj_ptr1());
  EXPECT_EQ(&cobj, cobj_ptr1());

  auto cobj_ref0 = cxx::detail::objref_impl<const int &, &cobj>();
  auto cobj_ref1 = CXX_OBJREF(cobj);
  EXPECT_EQ(4, cobj_ref0());
  EXPECT_EQ(4, cobj_ref1());
  EXPECT_EQ(&cobj, &cobj_ref1());

  // Cannot wrap obj itself since obj is not const

  auto obj_ptr0 = cxx::detail::obj_impl<int *, &obj>();
  auto obj_ptr1 = CXX_OBJ(&obj);
  EXPECT_EQ(5, *obj_ptr0());
  EXPECT_EQ(5, *obj_ptr1());
  EXPECT_EQ(&obj, obj_ptr1());
  *obj_ptr1() = 5;

  auto obj_ref0 = cxx::detail::objref_impl<int &, &obj>();
  auto obj_ref1 = CXX_OBJREF(obj);
  EXPECT_EQ(5, obj_ref0());
  EXPECT_EQ(5, obj_ref1());
  EXPECT_EQ(&obj, &obj_ref1());
  obj_ref1() = 5;
}
