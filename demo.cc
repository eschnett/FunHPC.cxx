#include "mpi_rpc.hh"

#include <boost/serialization/string.hpp>
#include <boost/serialization/strong_typedef.hpp>
#include <boost/utility/identity_type.hpp>

#include <cstdio>
#include <future>
#include <iostream>
#include <string>
#include <type_traits>

using std::cout;
using std::flush;
using std::printf;
using std::string;



int f(int n)
{
  cout << "--> Called f(n=" << n << ") on process " << rpc::comm.rank() << "\n";
  return n+1;
}
struct f_action {
  int operator()(int arg0) const { return f(arg0); }
};
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::action_evaluate<f_action, int, int>)));
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::action_finish<f_action, int>)));

// BOOST_STRONG_TYPEDEF(decltype(&f), f_type);
// f_type f1(&f);
// auto f2 = *f1;
// auto f3 = *f_type(&f);
// const f_type& f4(*f_type(&f));
// typedef const f_type& f_type2;
// f_type2 f5(*f_type(&f));



int add(int m, int n)
{
  return m+n;
}
struct add_action {
  int operator()(int arg0, int arg1) const { return add(arg0, arg1); }
};
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::action_evaluate<add_action, int, int, int>)));
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::action_finish<add_action, int>)));



void out(string str)
{
  printf("%s\n", str.c_str());
}
struct out_action {
  void operator()(string arg0) const { return out(arg0); }
};
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::action_evaluate<out_action, void, string>)));
BOOST_CLASS_EXPORT(BOOST_IDENTITY_TYPE((rpc::action_finish<out_action, void>)));



int rpc_main(int argc, char** argv)
{
  cout << "Calling f directly... " << flush;
  cout << f(10) << "\n";
  // cout << (*f1)(10) << "\n";
  // cout << f2(10) << "\n";
  // cout << f3(10) << "\n";
  // cout << f4(10) << "\n";
  // cout << f5(10) << "\n";
  cout << "Calling f as action... " << flush;
  cout << f_action()(20) << "\n";
  cout << "Calling f synchronously... " << flush;
  cout << rpc::sync(f_action(), 40) << "\n";
  cout << "Calling f asynchronously... " << flush;
  cout << rpc::async(f_action(), 30).get() << "\n";
  cout << "Calling f applicatively...\n" << flush;
  rpc::apply(f_action(), 50);
  cout << "Done calling f\n";
  
  cout << "Calling add directly... " << flush;
  cout << add(1,2) << "\n";
  cout << "Calling add as action... " << flush;
  cout << add_action()(1,2) << "\n";
  cout << "Calling add synchronously... " << flush;
  cout << rpc::sync(add_action(), 1,2) << "\n";
  cout << "Calling add asynchronously... " << flush;
  cout << rpc::async(add_action(), 1,2).get() << "\n";
  cout << "Calling add applicatively...\n" << flush;
  rpc::apply(add_action(), 1,2);
  cout << "Done calling add\n";
  
  cout << "Calling out directly...\n" << flush;
  out(string("hello"));
  cout << "Calling out as action...\n" << flush;
  out_action()(string("hello"));
  cout << "Calling out synchronously...\n" << flush;
  rpc::sync(out_action(), string("hello"));
  cout << "Calling out asynchronously...\n" << flush;
  rpc::async(out_action(), string("hello")).get();
  cout << "Calling out applicatively...\n" << flush;
  rpc::apply(out_action(), string("hello"));
  cout << "Done calling out\n";
  
  // int dest = (rpc::rank() + 1) % rpc::size();
  // cout << "Calling f synchronously on process " << dest << "...\n";
  // sync(f_action, dest);
  
  return 0;
}
