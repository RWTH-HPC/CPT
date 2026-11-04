#pragma once

namespace __cpt {

template <typename T, typename Compare> class Set {
private:
  RBTree<T, Compare> tree;

public:
  using iterator = typename RBTree<T, Compare>::iterator;

  Set(Compare c) : tree(c) {}

  iterator begin() { return tree.begin(); }
  iterator end() { return tree.end(); }

  iterator lower_bound(const T &v) { return tree.lower_bound(v); }

  iterator insert(const T &v) { return tree.insert(v); }

  iterator erase(iterator it) { return tree.erase(it); }

  size_t size() const { return tree.size(); }

  void clear() { tree.clear(); }

  void *operator new(size_t size) { return malloc(size); }
  void operator delete(void *p) { free(p); }
};

} // namespace __cpt