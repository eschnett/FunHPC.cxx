int f(int x) { return x; }
template <typename T> T call_f(T x) { return f(x); }
double f(double x) { return x; }

double calling_f(double x) { return call_f(x); }

////////////////////////////////////////////////////////////////////////////////

template <typename T> struct q;
template <> struct q<int> {
  static int g(int x) { return x; }
};
template <typename T> T call_g(T x) { return q<T>::g(x); }
template <> struct q<double> {
  static double g(double x) { return x; }
};

double calling_g(double x) { return call_g(x); }

////////////////////////////////////////////////////////////////////////////////

template <typename T> int h(T, int x) { return x; }
template <typename T> T call_h(T x) { return h(T(), x); }
template <typename T> double h(T, double x) { return x; }

double calling_h(double x) { return call_h(x); }
