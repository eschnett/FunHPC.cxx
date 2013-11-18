#include "mpi_rpc.hh"

#include <boost/archive/polymorphic_text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>

#include <cstdio>
#include <future>
#include <iostream>
#include <sstream>

using std::cout;
using std::flush;
using std::istringstream;
using std::ostringstream;
using std::printf;



int f(int n)
{
  cout << "--> Called f(n=" << n << ") on process " << rpc::comm.rank() << "\n";
  return n+1;
}
// struct f_action: public rpc::action_base<f_action> {
//   int operator()(int arg0) const { return f(arg0); }
// private:
//   friend class boost::serialization::access;
//   template<typename Archive>
//   void serialize(Archive& ar, unsigned int file_version)
//   {
//     ar & boost::serialization::base_object<action_base<f_action>>(*this);
//   }
// };
struct f_action {
  int operator()(int arg0) const { return f(arg0); }
};

int add(int m, int n)
{
  return m+n;
}
// struct add_action: public rpc::action_base<add_action> {
//   int operator()(int arg0, int arg1) const { return add(arg0, arg1); }
// };
struct add_action {
  int operator()(int arg0, int arg1) const { return add(arg0, arg1); }
};

void out(const char* str)
{
  printf("%s\n", str);
}
// struct out_action: public rpc::action_base<out_action> {
//   void operator()(const char* arg0) const { return out(arg0); }
// };
struct out_action {
  void operator()(const char* arg0) const { return out(arg0); }
};



int rpc_main(int argc, char** argv)
{
  cout << "Calling f directly... " << flush;
  cout << f(10) << "\n";
  cout << "Calling f as action... " << flush;
  cout << f_action()(20) << "\n";
  cout << "Calling f asynchronously... " << flush;
  cout << rpc::async(f_action(), 30).get() << "\n";
  cout << "Calling f synchronously... " << flush;
  cout << rpc::sync(f_action(), 40) << "\n";
  cout << "Calling f applicatively...\n" << flush;
  rpc::apply(f_action(), 50);
  cout << "Done calling f\n";
  
  cout << "Calling add directly... " << flush;
  cout << add(1,2) << "\n";
  cout << "Calling add as action... " << flush;
  cout << add_action()(1,2) << "\n";
  cout << "Calling add asynchronously... " << flush;
  cout << rpc::async(add_action(), 1,2).get() << "\n";
  cout << "Calling add synchronously... " << flush;
  cout << rpc::sync(add_action(), 1,2) << "\n";
  cout << "Calling add applicatively...\n" << flush;
  rpc::apply(add_action(), 1,2);
  cout << "Done calling add\n";
  
  cout << "Calling out directly...\n" << flush;
  out("hello");
  cout << "Calling out as action...\n" << flush;
  out_action()("hello");
  cout << "Calling out asynchronously...\n" << flush;
  rpc::async(out_action(), "hello").get();
  cout << "Calling out synchronously...\n" << flush;
  rpc::sync(out_action(), "hello");
  cout << "Calling out applicatively...\n" << flush;
  rpc::apply(out_action(), "hello");
  cout << "Done calling out\n";
  
  // int dest = (rpc::rank() + 1) % rpc::size();
  // cout << "Calling f synchronously on process " << dest << "...\n";
  // sync(f_action, dest);
  
  return 0;
}
