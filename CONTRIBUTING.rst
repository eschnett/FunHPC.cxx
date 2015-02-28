How to contribute to FunHPC.cxx
===============================

I welcome contributions to FunHPC.cxx. The easiest way to do so is via
Bitbucket pull requests.

Directory layout
----------------

- cxx: Features that should arguably be in the C++ standard
- qthread: Replacements for the STL headers `<future>`, `<mutex>`, and
  `<thread>`, which provide the C++ multi-threading capabilities,
  implemented on top of Qthreads
- funhpc: Extensions for the above for distributed multi-threading,
  implemented on top of MPI
- adt: Some data types that are generally useful, e.g. `adt::maybe`
  (aka `optional`), `adt::either` (type-safe union), a tree, and
  nesting containers
- fun: Functional programming in C++, i.e. functional equivalent to
  the iterator-based container model that C++ uses, and which doesn't
  work well for distributed memory systems: `iotaMap`, `fmap`,
  `foldMap`, etc.
- examples: Some examples
- External libraries:
  - external: Download and build directories for all external libraries:
  - cereal-*: Object serialization
  - gtest-*: Google Test framework
  - hwloc-*: Portable hardward locality
  - jemalloc-*: Efficient multi-threaded malloc replacement
  - qthread-*: Fine-grained multi-threading

Bikeshedding
------------

The source code of FunHPC.cxx is formatted via `clang-format`. This is
currently even done automatically in the Makefile before every build,
or when building the `make format` pseudo target.

C++ is a difficult language
---------------------------

C++ is a difficult language. No really, it is. While using FunHPC is
fairly straightforward -- not much more difficult than using the C++11
STL -- contributing to FunHPC is probably quite a bit more difficult.
I wish it was different. The problem is that FunHPC needs to handle
all sorts of special cases that someone will potentially use, while a
user of FunHPC only needs to handle those cases they are actually
using.

If you want to contribute to FunHPC.cxx, then you should be familiar
with the following C++ mechanisms:
- rvalue references (`&&`)
- moving values, as opposed to copying them (`std::move`)
- perfect forwarding (`std::forward`)
- template argument matching
- argument decay (`std::decay`)
- template argument packs (`typename... Args`)
- lambda expressions
- SFINAE (`std::enable_if`)

Each of these mechanisms is non-trivial. I recommend
<http://en.cppreference.com/> as a starting point to learn about
these. I will also be happy to explain how these influence FunHPC.cxx.
Note that there's nothing special about FunHPC.cxx; every modern C++
library will use these mechanisms.

The C++ language has certain "missing pieces". Many of these are
currently being debated in the C++ standards committee. I have taken
some of the working notes and proposed standard additions, and make
them available in FunHPC.cxx. These are located in the `cxx`
subdirectory. In brief, these are:
- `invoke.hpp`: Provide a function `cxx::invoke` and a type template
  `cxx::invoke_of`. `invoke` allows calling arbitrary callable
  entities (as defined by the C++ standard) with a uniform syntax. For
  example, functions, function pointers, and function objects are
  invoked via the syntax `f(x,y)`, while e.g. member functions are
  invoked via `x.f(y)`, member function pointers via `x.*f(y)`, etc.
  Using `invoke`, the syntax for all cases above is
  `cxx::invoke(f,x,y)`. This greatly simplifies implementing functions
  that expect a callable argument. The type template `invoke_of`
  allows determining the return type of such a call in an SFINAE
  friendly manner.
- `apply.hpp`: Provide a function `cxx::apply` that calls a function
  with arguments, where the arguments are given as a tuple that needs
  to be deconstructed. For example, given a function `f` and a tuple
  `t = std::make_tuple(x,y)`, `cxx::apply(f,t)` evaluates `f(x,y)`.
- `task.hpp`: A mixture of `std::bind`, `std::unique_function`,
  `std::packaged_task`. Note that `unique_function` does not actually
  exist in the C++ standard, but one can easily deduce approximately
  how it would look like if it did. If `unique_function` existed, this
  class `task` was not necessary.
- `utility.hpp`: Provide template "operators" `cxx::all_of_type`,
  `cxx::any_of_type`, and `cxx::none_of_type`. These are equivalent to
  `std::all_of` etc., except that they act on template argument packs
  instead of containers. For example,
  `cxx::any_of_type<std::true_type, std::false_type>` is equivalent to
  `std::true_type`.
- `cstdlib.hpp`: Provide small helper functions for integer
  arithmetic, e.g. integer division that always rounds towards minus
  or plus infinity.
- `serialize.hpp`: Extend the Cereal library with a serialization
  mechanism for function pointers, member function pointers, and
  member object pointers. This functionality is system dependent; as
  implemented, it is correct for x86-based Unix systems. Support for
  other architecturs could easily be added.

To contribute to FunHPC.cxx, it is necessary to understand the
mechanisms in the `cxx` directory, since they are used ubiquituously.
