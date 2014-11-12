inline char init() { return 'a'; }

template <typename T> struct s {
  static char x;
  char y;
  static const char z = 'a';
  void f() {}
  static void g() {}
};
template <typename T> char s<T>::x = init();

char f() { return s<int>::x; }

int main() {}
