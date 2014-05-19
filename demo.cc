#include "rpc.hh"

#include <boost/serialization/access.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using rpc::future;
using rpc::mutex;
using rpc::shared_future;

using boost::make_shared;
using boost::shared_ptr;

using std::cerr;
using std::cout;
using std::flush;
using std::printf;
using std::string;
using std::vector;



int f(int n)
{
  cout << "--> Called f(n=" << n << ") "
       << "on process " << rpc::server->rank() << " "
       << "of " << rpc::server->size() << "\n";
  return n+1;
}
RPC_ACTION(f);



int add(int m, int n)
{
  return m+n;
}
RPC_ACTION(add);



void out(string str)
{
  printf("%s\n", str.c_str());
}
RPC_ACTION(out);



struct point {
  int x, y;
  void init(int value) { x=y=value; }
  RPC_DECLARE_MEMBER_ACTION(point, init);
  void translate(rpc::client<point> delta_)
  {
    auto delta = delta_.make_local();
    x+=delta->x; y+=delta->y;
  }
  RPC_DECLARE_MEMBER_ACTION(point, translate);
  void output() const
  {
    rpc::with_lock(rpc::io_mutex, [&]{
        cout << "[" << rpc::server->rank() << "] "
             << "point(" << x << "," << y << ")\n";
      });
  }
  RPC_DECLARE_CONST_MEMBER_ACTION(point, output);
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, unsigned int version)
  {
    ar & x & y;
  }
};
RPC_COMPONENT(point);
RPC_IMPLEMENT_MEMBER_ACTION(point, init);
RPC_IMPLEMENT_MEMBER_ACTION(point, translate);
RPC_IMPLEMENT_CONST_MEMBER_ACTION(point, output);



void test_call()
{
  int dest = 1 % rpc::server->size();
  
  cout << "Calling f directly... " << flush;
  cout << f(10) << "\n";
  cout << "Calling f as action... " << flush;
  cout << f_action()(20) << "\n";
  cout << "Calling f synchronously... " << flush;
  cout << rpc::sync(dest, f_action(), 30) << "\n";
  cout << "Calling f asynchronously... " << flush;
  cout << rpc::async(dest, f_action(), 40).get() << "\n";
  cout << "Calling f detached...\n" << flush;
  rpc::detached(dest, f_action(), 50);
  cout << "Done calling f\n";
  
  cout << "Calling add directly... " << flush;
  cout << add(1,2) << "\n";
  cout << "Calling add as action... " << flush;
  cout << add_action()(1,2) << "\n";
  cout << "Calling add synchronously... " << flush;
  cout << rpc::sync(dest, add_action(), 1,2) << "\n";
  cout << "Calling add asynchronously... " << flush;
  cout << rpc::async(dest, add_action(), 1,2).get() << "\n";
  cout << "Calling add detached...\n" << flush;
  rpc::detached(dest, add_action(), 1,2);
  cout << "Done calling add\n";
  
  cout << "Calling out directly...\n" << flush;
  out("hello");
  cout << "Calling out as action...\n" << flush;
  out_action()("hello");
  cout << "Calling out synchronously...\n" << flush;
  rpc::sync(dest, out_action(), string("hello"));
  cout << "Calling out asynchronously...\n" << flush;
  rpc::async(dest, out_action(), string("hello")).get();
  cout << "Calling out detached...\n" << flush;
  rpc::detached(dest, out_action(), string("hello"));
  cout << "Done calling out\n";
  
  auto p = rpc::make_client<point>();
  auto q = rpc::make_client<point>();
  rpc::sync(point::init_action(), p, 1);
  rpc::sync(point::init_action(), q, 2);
  rpc::sync(point::translate_action(), p, q);
  rpc::sync(point::output_action(), p);
  
  auto rp = rpc::make_remote_client<point>(1 % rpc::server->size());
  auto rq = rpc::make_remote_client<point>(2 % rpc::server->size());
  rpc::sync(point::init_action(), rp, 3);
  rpc::sync(point::init_action(), rq, 4);
  rpc::sync(point::translate_action(), rp, rq);
  rpc::sync(point::output_action(), rp);
}



struct s {
  int i;
  s(): i(-1)
  {
    cout << "[" << rpc::server->rank() << "] "
         << "Default-constructing s() at " << this << "\n" << flush;
  }
  s(int i): i(i)
  {
    cout << "[" << rpc::server->rank() << "] "
         << "Constructing s(" << i << ") at " << this << "\n" << flush;
  }
  s(const s& other): i(other.i)
  {
    cout << "[" << rpc::server->rank() << "] "
         << "Copy-constructing s(" << i << ") at " << this << "\n" << flush;
  }
  s(s&& other): i(other.i)
  {
    cout << "[" << rpc::server->rank() << "] "
         << "Move-constructing s(" << i << ") at " << this << "\n" << flush;
  }
  s& operator=(const s& other)
  {
    if (this == &other) return *this;
    cout << "[" << rpc::server->rank() << "] "
         << "Assigning from s(" << other.i << ") to s(" << i << ") at " << this << "\n" << flush;
    i = other.i;
    return *this;
  }
  s& operator=(s&& other)
  {
    RPC_ASSERT(this != &other);
    cout << "[" << rpc::server->rank() << "] "
         << "Moving from s(" << other.i << ") to s(" << i << ") at " << this << "\n" << flush;
    i = other.i;
    return *this;
  }
  ~s()
  {
    RPC_ASSERT(i>=0);
    cout << "[" << rpc::server->rank() << "] "
         << "Destructing s(" << i << ") at " << this << "\n" << flush;
    i = -2;
  }
private:
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive& ar, unsigned int version) const
  {
    cout << "[" << rpc::server->rank() << "] "
         << "Saving s(" << i << ") at " << this << "\n" << flush;
    ar << i;
  }
  template<class Archive>
  void load(Archive& ar, unsigned int version)
  {
    ar >> i;
    cout << "[" << rpc::server->rank() << "] "
         << "Loading s(" << i << ") at " << this << "\n" << flush;
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER();
};

// TODO: use const&
void tpc(shared_ptr<s> is,
         rpc::global_ptr<s> ig,
         rpc::global_shared_ptr<s> igs)
{
}
RPC_ACTION(tpc);

void tgsp(rpc::global_shared_ptr<s> igs, ptrdiff_t count, ptrdiff_t level = 0);
RPC_ACTION(tgsp);
void tgsp(rpc::global_shared_ptr<s> igs, ptrdiff_t count, ptrdiff_t level)
{
  if (count == 0) return;
  rpc::with_lock(rpc::io_mutex, [&]{
      cout << "[" << rpc::server->rank() << "] "
           << "tgsp " << count << " " << level << "\n";
    });
  int nchildren = 3;
  vector<future<void> > fs;
  for (int child=0; child<nchildren; ++child) {
    ptrdiff_t child_count = count / 4;
    int dest = (rpc::server->rank() + 1001 + 5 * child) % rpc::server->size();
    fs.push_back(async(dest, tgsp_action(), igs, child_count, level+1));
    count -= child_count;
  }
  vector<rpc::global_shared_ptr<s> > locals(count, igs);
  // rpc::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (auto& f: fs) f.wait();
}



void test_ptr()
{
  int dest = 1 % rpc::server->size();
  
  auto ip = new s(1);
  auto is = make_shared<s>(2);
  auto ig = rpc::make_global<s>(3);
  auto igs = rpc::make_global_shared<s>(4);
  
  auto ip2 = ip;
  auto is2 = is;
  auto ig2 = ig;
  auto igs2 = igs;
  
  tpc(is, ig, igs);
  rpc::sync(dest, tpc_action(), is, ig, igs);
  tgsp(rpc::make_global_shared<s>(5), 100000);
  
  delete ip2;
  delete ig2.get();
}


  
int rpc_main(int argc, char** argv)
{
  test_call();
  test_ptr();
  return 0;
}
