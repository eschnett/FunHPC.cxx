inline char init() { return 'a'; }

template <typename T> struct s {
  static char x;
  char y;
  void f() {}
  static void g() {}
};
template <typename T> char s<T>::x = init();

char f2() { return s<int>::x; }
