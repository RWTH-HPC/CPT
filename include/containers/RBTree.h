#pragma once

namespace __cpt {

template <typename T, typename Compare> class RBTree {
private:
  enum Color { RED, BLACK };

  struct Node {
    T value;
    Color color;
    Node *parent;
    Node *left;
    Node *right;

    Node(const T &v)
        : value(v), color(RED), parent(nullptr), left(nullptr), right(nullptr) {
    }

    void *operator new(size_t size) { return malloc(size); }
    void operator delete(void *p) { free(p); }
  };

  Node *root = nullptr;
  Compare cmp;
  size_t node_count = 0;

public:
  class iterator {
  public:
    Node *node;
    Node *root;

    iterator(Node *n = nullptr, Node *r = nullptr) : node(n), root(r) {}

    void *operator new(size_t size) { return malloc(size); }
    void operator delete(void *p) { free(p); }

    T &operator*() { return node->value; }
    T *operator->() { return &node->value; }

    bool operator==(const iterator &o) const { return node == o.node; }
    bool operator!=(const iterator &o) const { return node != o.node; }

    static Node *minimum(Node *n) {
      while (n && n->left)
        n = n->left;
      return n;
    }

    static Node *maximum(Node *n) {
      while (n && n->right)
        n = n->right;
      return n;
    }

    static Node *successor(Node *n) {
      if (n->right)
        return minimum(n->right);

      Node *p = n->parent;
      while (p && n == p->right) {
        n = p;
        p = p->parent;
      }
      return p;
    }

    static Node *predecessor(Node *n) {
      if (n->left)
        return maximum(n->left);

      Node *p = n->parent;
      while (p && n == p->left) {
        n = p;
        p = p->parent;
      }
      return p;
    }

    iterator &operator++() {
      node = successor(node);
      return *this;
    }

    iterator &operator--() {
      if (node == nullptr) { // --end()
        node = maximum(root);
        return *this;
      }
      node = predecessor(node);
      return *this;
    }
  };

private:
  static Node *minimum(Node *n) {
    while (n && n->left)
      n = n->left;
    return n;
  }

  void rotate_left(Node *x) {
    Node *y = x->right;
    x->right = y->left;
    if (y->left)
      y->left->parent = x;

    y->parent = x->parent;

    if (!x->parent)
      root = y;
    else if (x == x->parent->left)
      x->parent->left = y;
    else
      x->parent->right = y;

    y->left = x;
    x->parent = y;
  }

  void rotate_right(Node *x) {
    Node *y = x->left;
    x->left = y->right;
    if (y->right)
      y->right->parent = x;

    y->parent = x->parent;

    if (!x->parent)
      root = y;
    else if (x == x->parent->right)
      x->parent->right = y;
    else
      x->parent->left = y;

    y->right = x;
    x->parent = y;
  }

  void fix_insert(Node *z) {
    while (z->parent && z->parent->color == RED) {
      if (z->parent == z->parent->parent->left) {
        Node *y = z->parent->parent->right;
        if (y && y->color == RED) {
          z->parent->color = BLACK;
          y->color = BLACK;
          z->parent->parent->color = RED;
          z = z->parent->parent;
        } else {
          if (z == z->parent->right) {
            z = z->parent;
            rotate_left(z);
          }
          z->parent->color = BLACK;
          z->parent->parent->color = RED;
          rotate_right(z->parent->parent);
        }
      } else {
        Node *y = z->parent->parent->left;
        if (y && y->color == RED) {
          z->parent->color = BLACK;
          y->color = BLACK;
          z->parent->parent->color = RED;
          z = z->parent->parent;
        } else {
          if (z == z->parent->left) {
            z = z->parent;
            rotate_right(z);
          }
          z->parent->color = BLACK;
          z->parent->parent->color = RED;
          rotate_left(z->parent->parent);
        }
      }
    }
    root->color = BLACK;
  }

  void transplant(Node *u, Node *v) {
    if (!u->parent)
      root = v;
    else if (u == u->parent->left)
      u->parent->left = v;
    else
      u->parent->right = v;

    if (v)
      v->parent = u->parent;
  }

  void fix_erase(Node *x, Node *x_parent) {
    while (x != root && (!x || x->color == BLACK)) {
      if (x == x_parent->left) {
        Node *w = x_parent->right;
        if (w && w->color == RED) {
          w->color = BLACK;
          x_parent->color = RED;
          rotate_left(x_parent);
          w = x_parent->right;
        }
        if (!w)
          break;
        if ((!w->left || w->left->color == BLACK) &&
            (!w->right || w->right->color == BLACK)) {
          w->color = RED;
          x = x_parent;
          x_parent = x->parent;
        } else {
          if (!w->right || w->right->color == BLACK) {
            if (w->left)
              w->left->color = BLACK;
            w->color = RED;
            rotate_right(w);
            w = x_parent->right;
          }
          w->color = x_parent->color;
          x_parent->color = BLACK;
          if (w->right)
            w->right->color = BLACK;
          rotate_left(x_parent);
          x = root;
        }
      } else {
        Node *w = x_parent->left;
        if (w && w->color == RED) {
          w->color = BLACK;
          x_parent->color = RED;
          rotate_right(x_parent);
          w = x_parent->left;
        }
        if (!w)
          break;
        if ((!w->right || w->right->color == BLACK) &&
            (!w->left || w->left->color == BLACK)) {
          w->color = RED;
          x = x_parent;
          x_parent = x->parent;
        } else {
          if (!w->left || w->left->color == BLACK) {
            if (w->right)
              w->right->color = BLACK;
            w->color = RED;
            rotate_left(w);
            w = x_parent->left;
          }
          w->color = x_parent->color;
          x_parent->color = BLACK;
          if (w->left)
            w->left->color = BLACK;
          rotate_right(x_parent);
          x = root;
        }
      }
    }
    if (x)
      x->color = BLACK;
  }

public:
  RBTree(Compare c) : cmp(c) {}

  typename RBTree::iterator begin() { return iterator(minimum(root), root); }
  typename RBTree::iterator end() { return iterator(nullptr, root); }

  typename RBTree::iterator lower_bound(const T &value) {
    Node *current = root;
    Node *result = nullptr;

    while (current) {
      if (!cmp(current->value, value)) {
        result = current;
        current = current->left;
      } else {
        current = current->right;
      }
    }
    return iterator(result, root);
  }

  typename RBTree::iterator insert(const T &value) {
    Node *parent = nullptr;
    Node *cur = root;

    while (cur) {
      parent = cur;
      if (cmp(value, cur->value))
        cur = cur->left;
      else if (cmp(cur->value, value))
        cur = cur->right;
      else
        return iterator(cur,
                        root); // Element already exists; return it immediately
    }

    Node *n = new Node(value);
    n->parent = parent;

    if (!parent)
      root = n;
    else if (cmp(value, parent->value))
      parent->left = n;
    else
      parent->right = n;

    fix_insert(n);
    node_count++;

    return iterator(n, root);
  }

  typename RBTree::iterator erase(iterator it) {
    if (!it.node)
      return it;

    Node *z = it.node;
    iterator next = it;
    ++next;

    Node *y = z;
    Color y_original_color = y->color;
    Node *x;
    Node *x_parent;

    if (!z->left) {
      x = z->right;
      x_parent = z->parent;
      transplant(z, z->right);
    } else if (!z->right) {
      x = z->left;
      x_parent = z->parent;
      transplant(z, z->left);
    } else {
      y = minimum(z->right);
      y_original_color = y->color;
      x = y->right;

      // x_parent logic ensures fix_erase operates safely even if x is nullptr
      if (y->parent == z) {
        x_parent = y;
      } else {
        x_parent = y->parent;
        transplant(y, y->right);
        y->right = z->right;
        y->right->parent = y;
      }

      transplant(z, y);
      y->left = z->left;
      y->left->parent = y;
      y->color = z->color;
    }

    delete z;
    node_count--;

    if (y_original_color == BLACK) {
      fix_erase(x, x_parent);
    }

    return next;
  }

  void clear_node(Node *n) {
    if (!n)
      return;

    clear_node(n->left);
    clear_node(n->right);
    delete n;
  }

  void clear() {
    clear_node(root);
    root = nullptr;
    node_count = 0;
  }

  size_t size() const { return node_count; }
};

} // namespace __cpt