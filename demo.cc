#include "rpc.hh"

#include <boost/serialization/string.hpp>

#include <cstdio>
#include <iostream>
#include <string>

using std::cout;
using std::flush;
using std::printf;
using std::string;



int f(int n)
{
  cout << "--> Called f(n=" << n << ") "
       << "on process " << rpc::server->rank() << " "
       << "of " << rpc::server->size() << "\n";
  return n+1;
}

struct f_action:
  public rpc::action_impl<f_action, rpc::wrap<decltype(f), f>>
{
};
BOOST_CLASS_EXPORT(f_action::evaluate);
BOOST_CLASS_EXPORT(f_action::finish);



int add(int m, int n)
{
  return m+n;
}

struct add_action:
  public rpc::action_impl<add_action, rpc::wrap<decltype(add), add>>
{
};
BOOST_CLASS_EXPORT(add_action::evaluate);
BOOST_CLASS_EXPORT(add_action::finish);



void out(string str)
{
  printf("%s\n", str.c_str());
}

struct out_action:
  public rpc::action_impl<out_action, rpc::wrap<decltype(out), out>>
{
};
BOOST_CLASS_EXPORT(out_action::evaluate);
BOOST_CLASS_EXPORT(out_action::finish);



int rpc_main(int argc, char** argv)
{
  cout << "Calling f directly... " << flush;
  cout << f(10) << "\n";
  cout << "Calling f as action... " << flush;
  cout << f_action()(20) << "\n";
  cout << "Calling f synchronously... " << flush;
  cout << rpc::sync(0, f_action(), 40) << "\n";
  cout << "Calling f asynchronously... " << flush;
  cout << rpc::async(0, f_action(), 30).get() << "\n";
  cout << "Calling f applicatively...\n" << flush;
  rpc::apply(0, f_action(), 50);
  cout << "Done calling f\n";
  
  cout << "Calling add directly... " << flush;
  cout << add(1,2) << "\n";
  cout << "Calling add as action... " << flush;
  cout << add_action()(1,2) << "\n";
  cout << "Calling add synchronously... " << flush;
  cout << rpc::sync(0, add_action(), 1,2) << "\n";
  cout << "Calling add asynchronously... " << flush;
  cout << rpc::async(0, add_action(), 1,2).get() << "\n";
  cout << "Calling add applicatively...\n" << flush;
  rpc::apply(0, add_action(), 1,2);
  cout << "Done calling add\n";
  
  cout << "Calling out directly...\n" << flush;
  out("hello");
  cout << "Calling out as action...\n" << flush;
  out_action()("hello");
  cout << "Calling out synchronously...\n" << flush;
  rpc::sync(0, out_action(), "hello");
  cout << "Calling out asynchronously...\n" << flush;
  rpc::async(0, out_action(), "hello").get();
  cout << "Calling out applicatively...\n" << flush;
  rpc::apply(0, out_action(), "hello");
  cout << "Done calling out\n";
  
  // int dest = (rpc::rank() + 1) % rpc::size();
  // cout << "Calling f synchronously on process " << dest << "...\n";
  // sync(0, f_action, dest);
  
  return 0;
}
