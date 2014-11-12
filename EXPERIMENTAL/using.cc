template <typename T> struct s { T x; };
template <typename T> using u = s<T>;

int a(s<int> y) { return y.x; }
int b(u<int> y) { return y.x; }

template <template <typename> class C> struct c { C<int> y; };

int f(c<s> z) { return z.y.x; }
int g(c<u> z) { return z.y.x; }
