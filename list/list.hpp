#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

template <typename T, typename Alloc = std::allocator<T>>
class List {
 public:
  struct fake_node;
  struct node;

  template <bool IsConst>
  class common_iterator;

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using allocator_type = Alloc;
  using alloc_traits = std::allocator_traits<
      typename std::allocator_traits<Alloc>::template rebind_alloc<node>>;
  using rebind_type =
      typename std::allocator_traits<Alloc>::template rebind_alloc<node>;
  using value_type = T;

  List() : size_(0) {}

  explicit List(size_t count, const Alloc& alloc = Alloc())
      : size_(0), alloc_(alloc) {
    (&base_)->next_ = (&base_)->prev_ = nullptr;
    for (size_t i = 0; i < count; ++i) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        if constexpr (std::is_default_constructible<T>::value) {
          alloc_traits::construct(alloc_, new_node);
          push_back_def(new_node);
        } else {
          push_back_def(new_node);
        }
      } catch (...) {
        destroy(new_node);
        throw;
      }
    }
  }

  List(size_t count, const T& value, const Alloc& alloc = Alloc())
      : size_(0), alloc_(alloc) {
    (&base_)->next_ = (&base_)->prev_ = nullptr;
    for (size_t i = 0; i < count; ++i) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        alloc_traits::construct(alloc_, new_node,
                                node(value, (&base_)->prev_, (&base_)->next_));
        push_back_def(new_node);
      } catch (...) {
        destroy(new_node);
        throw;
      }
    }
  }

  List(const List& other)
      : size_(0),
        alloc_(
            alloc_traits::select_on_container_copy_construction(other.alloc_)) {
    (&base_)->next_ = (&base_)->prev_ = nullptr;
    for (node* counter = other.base_.next_;
         counter != (other.base_.prev_)->next_; counter = counter->next_) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        alloc_traits::construct(alloc_, new_node, counter->value_, nullptr,
                                nullptr);
        push_back_def(new_node);
      } catch (...) {
        destroy(new_node);
        throw;
      }
    }
  }

  List(std::initializer_list<T> init, const Alloc& alloc = Alloc())
      : size_(0), alloc_(alloc) {
    (&base_)->next_ = (&base_)->prev_ = nullptr;
    for (auto iter = init.begin(); iter != init.end(); ++iter) {
      node* new_node = alloc_traits::allocate(alloc_, 1);
      try {
        alloc_traits::construct(alloc_, new_node, *iter, (&base_)->prev_,
                                (&base_)->next_);
        push_back_def(new_node);
      } catch (...) {
        destroy(new_node);
        throw;
      }
    }
  }

  ~List() { clear(); }
  List& operator=(const List& other) {
    if (this != &other) {
      List copy = other;
      if (std::allocator_traits<
              Alloc>::propagate_on_container_copy_assignment::value) {
        alloc_ = other.alloc_;
      }
      clear();
      list_swap(copy);
    }
    return *this;
  }

  void push_back(const T& value) {
    try {
      node* new_node = create_node(value);
      if ((&base_)->next_ == nullptr) {
        permutation(new_node);
      } else {
        permutation_back(new_node);
      }
      ++size_;
    } catch (...) {
      throw;
    }
  }

  void push_back_def(node* new_node) {
    if ((&base_)->next_ == nullptr) {
      permutation(new_node);
    } else {
      permutation_back(new_node);
    }
    ++size_;
  }

  void push_back(T&& value) {
    node* new_node;
    if constexpr (std::is_move_constructible<T>::value) {
      new_node = create_node(std::move(value));
    } else {
      new_node = create_node(value);
    }
    if ((&base_)->next_ == nullptr) {
      permutation(new_node);
    } else {
      permutation_back(new_node);
    }
    ++size_;
  }

  void push_front(const T& value) {
    try {
      node* new_node = create_node(value);
      if ((&base_)->next_ != nullptr) {
        permutation_front(new_node);
      } else {
        permutation(new_node);
      }
      ++size_;
    } catch (...) {
      throw;
    }
  }

  void push_front(T&& value) {
    node* new_node;
    if constexpr (std::is_move_constructible<T>::value) {
      new_node = create_node(std::move(value));
    } else {
      new_node = create_node(value);
    }
    if ((&base_)->next_ != nullptr) {
      permutation_front(new_node);
      new_node->prev_ = reinterpret_cast<node*>(&base_);
    } else {
      permutation(new_node);
    }
    ++size_;
  }

  void pop_back() {
    if (size_ != 0) {
      if (size_ == 1) {
        destroy_node((&base_)->next_);
        base_.prev_ = base_.next_ = nullptr;
      } else {
        node* prev_node = ((&base_)->prev_)->prev_;
        destroy_node((&base_)->prev_);
        prev_node->next_ = static_cast<node*>((&base_));
        (&base_)->prev_ = prev_node;
      }
      --size_;
    }
  }

  void pop_front() {
    if (size_ != 0) {
      if (size_ == 1) {
        destroy_node((&base_)->next_);
        base_.prev_ = base_.next_ = nullptr;
      } else {
        node* next_node = (&base_)->next_->next_;
        destroy_node((&base_)->next_);
        next_node->prev_ = reinterpret_cast<node*>(&base_);
        (&base_)->next_ = next_node;
      }
      --size_;
    }
  }

  size_t size() const { return size_; }
  bool empty() { return (size_ == 0); }

  iterator begin() { return iterator((&base_)->next_); }
  const_iterator begin() const { return const_iterator((&base_)->next_); }

  iterator end() { return iterator(reinterpret_cast<node*>(&base_)); }

  const_iterator end() const {
    return const_iterator(reinterpret_cast<node*>(&base_));
  }

  const rebind_type& get_allocator() const { return alloc_; }

  const_iterator cend() const {
    return const_iterator(reinterpret_cast<node*>(&base_));
  }
  const_iterator cbegin() const { return const_iterator((&base_)->next_); }

  reverse_iterator rbegin() { return std::make_reverse_iterator(end()); }
  reverse_iterator rend() { return std::make_reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const {
    return std::make_reverse_iterator(cend());
  }
  const_reverse_iterator rend() const {
    return std::make_reverse_iterator(cbegin());
  }

  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cend());
  }
  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }

  T& front() { return (&base_)->next_->value_; }
  const T& front() const { return (&base_)->next_->value_; }
  T& back() { return (&base_)->prev_->value_; }
  const T& back() const { return (&base_)->prev_->value_; }

 private:
  mutable fake_node base_ = fake_node();
  int64_t size_;
  rebind_type alloc_;

  void permutation(node* new_node) {
    (&base_)->prev_ = (&base_)->next_ = new_node;
    new_node->prev_ = new_node->next_ = reinterpret_cast<node*>(&base_);
  }
  void permutation_back(node* new_node) {
    ((&base_)->prev_)->next_ = new_node;
    new_node->prev_ = (&base_)->prev_;
    (&base_)->prev_ = new_node;
    new_node->next_ = reinterpret_cast<node*>(&base_);
  }
  void permutation_front(node* new_node) {
    (&base_)->next_->prev_ = new_node;
    new_node->next_ = (&base_)->next_;
    (&base_)->next_ = new_node;
    new_node->prev_ = reinterpret_cast<node*>(&base_);
  }
  void destroy(node* new_node) {
    alloc_traits::deallocate(alloc_, new_node, 1);
    clear();
  }

  void list_swap(List& copy) {
    std::swap(size_, copy.size_);
    std::swap(base_, copy.base_);
    (base_.prev_)->next_ = reinterpret_cast<node*>(&base_);
    (base_.next_)->prev_ = reinterpret_cast<node*>(&base_);
  }

  void clear() {
    while (size_ != 0) {
      pop_back();
    }
  }

  node* create_node(const T& value) {
    node* new_node;
    try {
      new_node = alloc_traits::allocate(alloc_, 1);
      alloc_traits::construct(alloc_, new_node, value, nullptr, nullptr);
    } catch (...) {
      alloc_traits::deallocate(alloc_, new_node, 1);
      throw;
    }
    return new_node;
  }

  node* create_node(T&& value) {
    node* new_node;
    try {
      new_node = alloc_traits::allocate(alloc_, 1);
      alloc_traits::construct(alloc_, new_node, std::move(value), nullptr,
                              nullptr);
    } catch (...) {
      alloc_traits::deallocate(alloc_, new_node, 1);
      throw;
    }
    return new_node;
  }

  void destroy_node(node* n) {
    alloc_traits ::destroy(alloc_, n);
    alloc_traits::deallocate(alloc_, n, 1);
  }
};

template <typename T, typename Alloc>
struct List<T, Alloc>::fake_node {
 public:
  fake_node() : prev_(nullptr), next_(nullptr) {}
  fake_node(node* prev, node* next) : prev_(prev), next_(next) {}

  friend class List;

 protected:
  node* prev_;
  node* next_;
};

template <typename T, typename Alloc>
struct List<T, Alloc>::node : public fake_node {
 public:
  node() {}
  node(node* prev, node* next) : fake_node(prev, next) {}
  node(const T& value, node* prev, node* next)
      : value_(value), fake_node(prev, next) {}

  friend class List;

 protected:
  T value_;
};

template <typename T, typename Alloc>
template <bool IsConst>
class List<T, Alloc>::common_iterator {
 public:
  using type = std::conditional_t<IsConst, const T, T>;
  using value_type = T;
  using difference_type = int64_t;
  using pointer = type*;
  using reference = type&;
  using iterator_category = std::bidirectional_iterator_tag;

  common_iterator() = default;
  common_iterator(node* ptr) : ptr_(ptr) {}
  common_iterator(const common_iterator& other) : ptr_(other.ptr_) {}

  type& operator*() { return ptr_->value_; }
  const type& operator*() const { return ptr_->value_; }

  type* operator->() { return &(ptr_->value_); }
  const type* operator->() const { return &(ptr_->value_); }
  common_iterator& operator=(const common_iterator& other) {
    ptr_ = other.ptr_;
    return *this;
  }

  bool operator==(const common_iterator& other) const {
    return ptr_ == other.ptr_;
  }
  bool operator!=(const common_iterator& other) const {
    return !(ptr_ == other.ptr_);
  }

  common_iterator<IsConst>& operator++() {
    ptr_ = ptr_->next_;
    return *this;
  }

  common_iterator<IsConst> operator++(int) {
    common_iterator tmp = *this;
    ++(*this);
    return tmp;
  }

  common_iterator<IsConst>& operator--() {
    ptr_ = ptr_->prev_;
    return *this;
  }

  common_iterator<IsConst> operator--(int) {
    common_iterator tmp = *this;
    --(*this);
    return tmp;
  }

 private:
  node* ptr_;
};
