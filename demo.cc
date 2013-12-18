#include "rpc.hh"

#include <boost/serialization/access.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>
#include <boost/shared_ptr.hpp>

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using boost::make_shared;
using boost::shared_ptr;

using std::cerr;
using std::cout;
using std::flush;
using std::future;
using std::mutex;
using std::printf;
using std::shared_future;
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
    auto delta = delta_.local();
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
  cout << rpc::sync(dest, f_action(), 40) << "\n";
  cout << "Calling f asynchronously... " << flush;
  cout << rpc::async(dest, f_action(), 30).get() << "\n";
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
  rpc::sync(dest, out_action(), "hello");
  cout << "Calling out asynchronously...\n" << flush;
  rpc::async(dest, out_action(), "hello").get();
  cout << "Calling out detached...\n" << flush;
  rpc::detached(dest, out_action(), "hello");
  cout << "Done calling out\n";
  
  auto p = rpc::make_client<point>();
  auto q = rpc::make_client<point>();
  rpc::sync(p, point::init_action(), 1);
  rpc::sync(q, point::init_action(), 2);
  rpc::sync(p, point::translate_action(), q);
  rpc::sync(p, point::output_action());
  
  auto rp = rpc::make_remote_client<point>(1 % rpc::server->size());
  auto rq = rpc::make_remote_client<point>(2 % rpc::server->size());
  rpc::sync(rp, point::init_action(), 3);
  rpc::sync(rq, point::init_action(), 4);
  rpc::sync(rp, point::translate_action(), rq);
  rpc::sync(rp, point::output_action());
}



struct s {
  int i;
  s(): i(-1)
  {
    cout << "Default-constructing s() at " << this << "\n" << flush;
  }
  s(int i): i(i)
  {
    cout << "Constructing s(" << i << ") at " << this << "\n" << flush;
  }
  s(const s& other): i(other.i)
  {
    cout << "Copy-constructing s(" << i << ") at " << this << "\n" << flush;
  }
  s& operator=(const s& other)
  {
    if (this == &other) return *this;
    cout << "Assigning s(" << i << ") at " << this << "\n" << flush;
    i = other.i;
    return *this;
  }
  ~s()
  {
    assert(i>=0);
    cout << "Destructing s(" << i << ") at " << this << "\n" << flush;
    i = -2;
  }
private:
  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive& ar, unsigned int version) const
  {
    cout << "Saving s(" << i << ") at " << this << "\n" << flush;
    ar << i;
  }
  template<class Archive>
  void load(Archive& ar, unsigned int version)
  {
    ar >> i;
    cout << "Loading s(" << i << ") at " << this << "\n" << flush;
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

int random_r()
{
  static mutex m;
  return rpc::with_lock(m, random);
}

void tgsp(rpc::global_shared_ptr<s> igs, ptrdiff_t count, ptrdiff_t level = 0);
RPC_ACTION(tgsp);
void tgsp(rpc::global_shared_ptr<s> igs, ptrdiff_t count, ptrdiff_t level)
{
  if (count == 0) return;
  rpc::with_lock(rpc::io_mutex, [&]{
      cout << "[" << rpc::server->rank() << "] "
           << "tgsp " << count << " " << level << "\n";
    });
  int nchildren = random_r() % 4;
  vector<future<void>> fs;
  for (int child=0; child<nchildren; ++child) {
    ptrdiff_t child_count = count==0 ? 0 : random_r() % count;
    int dest = random_r() % rpc::server->size();
    fs.push_back(async(dest, tgsp_action(), igs, child_count, level+1));
    count -= child_count;
  }
  vector<rpc::global_shared_ptr<s>> locals(count, igs);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  for (auto& f: fs) f.wait();
}



void test_ptr()
{
  int dest = 1 % rpc::server->size();
  
  auto ip = new s(1);
  auto is = make_shared<s>(2);
  auto ig = rpc::make_global<s>(3);
  auto igs = rpc::make_shared_global<s>(4);
  
  auto ip2 = ip;
  auto is2 = is;
  auto ig2 = ig;
  auto igs2 = igs;
  
  tpc(is, ig, igs);
  rpc::sync(dest, tpc_action(), is, ig, igs);
  tgsp(rpc::make_shared_global<s>(5), 1000000);
  
  delete ip2;
  delete ig2.get();
}


  
int rpc_main(int argc, char** argv)
{
  test_call();
  test_ptr();
  return 0;
}
