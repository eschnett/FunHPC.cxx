================================================================================
MPI-RPC: Fine-grained multi-threading for distributed-memory systems
================================================================================
:Author: Erik Schnetter <eschnetter@perimeterinstitute.ca>
:Date: 2014-05-16



Overview
========

MPI-RPC is a C++ library that provides fine-grained multi-threading
for distributed-memory systems. The multi-threading API is very
similar to C++11, but extended to handle multiple processes with
separate address spaces. MPI-RPC is heavily influenced by HPX
<http://stellar.cct.lsu.edu/tag/hpx/>, and could be considered to be
an "HPX light".

MPI-RPC provides two major features: Executing functions on a remote
process, and pointers to objects living on a remote process. Using
C++11 syntax, these features look and behave very similar to C++11's
multi-threading and shared pointer APIs.

MPI-RPC implements a PGAS model -- a Partitioned Global Address Space.
This means that each C++ object can refer to any other C++ object,
independent of the process where the other C++ object lives. However,
the distinction between the different address spaces is kept explicit,
i.e. the programmer still has a notion of *local* vs. *global*
operations. This is important for achieving good performance.



Requirements
============

MPI-RPC uses several widely-used libraries to implement its features:

1. MPI: As the name indicates, communication between processes is
   handled via MPI.
2. Boost: Boost offers many interesting extensions to the C++
   standard. In addition, Boost.MPI and Boost.Serialize greatly
   simplify communicating arbitrary C++ objects between processes.
3. Qthreads <http://www.cs.sandia.gov/qthreads/>: Qthreads is
   developed at the Sandia National Laboratories, and provides
   efficient (local) fine-grained multi-threading.

Other libraries such as hwloc, gperftools, or PAPI are also useful to
understand or improve performance.



Programming
===========

Programming with MPI-RPC is probably best understood by starting from
C++11's multi-threading and memory-management features.

Memory management
-----------------

C++11 offers *shared pointers* and *unique pointers* to manage memory.
These can (in many cases) completely replace calling ``new`` and
``delete`` explicitly. Instead, shared pointers use reference
counting, and unique pointers ensure that there exists only a single
pointer to the object, which is automatically destructed when the
pointer is destructed. To create an object that is managed in such a
way, one calls ``make_shared`` or ``make_unique``.

.. MPI-RPC offers *global pointers*, which can point to objects living
   on any process. A global pointer can either be created from a local
   pointer via ``global_ptr<T>(T* ptr)``, or by calling a constructor
   for type ``T`` via ``make_global<T>(...)``. Note that global
   pointers are *not* reference counted.

MPI-RPC offers *global shared pointers*, which can point to objects
living on any process. A global shared pointer can either be created
from a (local) shared pointer via ``global_shared_ptr<T>(shared_ptr<T>
ptr)``, or by calling a constructor for type ``T`` via
``make_global_shared<T>(...)``. Global shared pointers are reference
counted in a way that is compatible with regular C++ shared pointers,
i.e. the respective object stayes alive as long as there are any
shared *or* global shared pointers referring to it.

MPI-RPC also offers *clients* (with a terminology borrowed from HPX),
which are futures to global shared pointers. Clients can be created
locally via ``make_client<T>(...)``, or remotely on a particular
process via ``make_remote_client(int proc, ...)``.

Multi-threading
---------------

C++11's threading facilities include *futures* and *shared futures*,
which are akin to unique pointers and shared pointers that block a
thread until the future holds a result. C++11 also provides *async*, a
facility to execute a function asynchronously that returns a future to
the function's result.

MPI-RPC extends ``async`` to be able to execute functions on other
processes. One can either specify the location directly (via an
additional argument), or one can use a global shared pointer or client
to call a member function which is then executed in the object's
process.

Serializing objects
-------------------

Objects that are communicated between processes -- i.e. that are used
as function arguments -- must be serialized. User-defined types that
need to be serialized need to use Boost.Serialize for this; the
details are documented there.

Note: Since ``std::shared_ptr`` and ``std::unique_ptr`` cannot be
serialized by Boost, you need to use ``boost:shared_ptr`` and
``boost::unique_ptr`` instead. These replacements have identical
behaviour.

Declaring functions globally
----------------------------

Functions that are called remotely must be registered globally in a
specific manner. MPI-RPC provides macros for this: ``RPC_ACTION`` for
regular functions, ``RPC_COMPONENT`` for classes, and
``RPC_DECLARE_MEMBER_ACTION`` and ``RPC_IMPLEMENT_MEMBER_ACTION`` for
member functions.



Examples
========

Define a function that can be called remotely:
----------------------------------------------

::

  int f(int n)
  {
    return n+1;
  }
  RPC_ACTION(f);

Call a function remotely:
-------------------------

::

  // call f directly
  int n1 = f(10);
  // call f as action
  int n2 = f_action()(20);
  // call f remotely, returning a future
  future<int> fn3 = rpc::async(dest, f_action(), 40);
  // call f remotely and wait for the result
  int n4 = rpc::sync(dest, f_action(), 30);

Define a class that can be serialized and communicated:
-------------------------------------------------------

::

  struct point {
    int x, y;
    void translate(rpc::client<point> delta);
    RPC_DECLARE_MEMBER_ACTION(point, translate);
    void output() const;
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
  RPC_IMPLEMENT_MEMBER_ACTION(point, translate);
  RPC_IMPLEMENT_CONST_MEMBER_ACTION(point, output);

Call member functions remotely:
-------------------------------

::

  auto p = make_client<point>();
  auto q = make_remote_client<point>(1);
  auto f1 = async(point::init_action(), p, 3);
  auto f2 = async(point::init_action(), q, 4);
  wait(f1); wait(f2);
  auto f3 = async(point::translate_action(), p, q);
  wait(f3);
  sync(point::output_action(), p);
