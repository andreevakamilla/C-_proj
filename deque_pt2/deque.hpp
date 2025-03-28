#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <type_traits>
#include <vector>

template <typename T, typename Alloc = std::allocator<T>>
class Deque {
 public:
  template <bool IsConst>
  class common_iterator;

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using alloc_traits = std::allocator_traits<Alloc>;

  using allocator_type = Alloc;
  using value_type = T;

  Deque() {}

  explicit Deque(const Alloc& alloc) : alloc_(alloc) {}

  Deque(size_t count, const Alloc& alloc = Alloc())
      : size_(0),
        chunk_((count + kChunkSize - 1) / kChunkSize),
        alloc_(alloc),
        chunk_num_((count + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    for (int64_t i = 0; i < chunk_num_; ++i) {
      try {
        chunk_[i] = alloc_traits::allocate(alloc_, kChunkSize);
      } catch (...) {
        alloc_traits::deallocate(alloc_, chunk_[i], kChunkSize);
        clear(finish_);
        throw;
      }
    }
    start_ = iterator(chunk_[0], &centre_, -centre_, &chunk_);
    finish_ = start_;
    if constexpr (std::is_default_constructible_v<T>) {
      while (size_ != count) {
        try {
          alloc_traits::construct(alloc_, (finish_).cur_);
          ++size_;
          ++finish_;
        } catch (...) {
          clear(finish_);
          throw;
        }
      }
    } else {
      size_ = count;
      finish_ += count;
    }
  }
  Deque(const Deque& other)
      : size_(0),
        chunk_((other.size_ + kChunkSize - 1) / kChunkSize),
        alloc_(
            alloc_traits::select_on_container_copy_construction(other.alloc_)),
        chunk_num_((other.size_ + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    if (other.size_ == 0) {
      def_deque();
    } else {
      for (int64_t i = 0; i < chunk_num_; ++i) {
        try {
          chunk_[i] = alloc_traits::allocate(alloc_, kChunkSize);
        } catch (...) {
          alloc_traits::deallocate(alloc_, chunk_[i], kChunkSize);
          clear(finish_);
          throw;
        }
      }
      start_ = iterator(chunk_[0], &centre_, -centre_, &chunk_);
      finish_ = start_;
      iterator copy = other.start_;
      if constexpr (std::is_default_constructible_v<T>) {
        while (size_ != other.size_) {
          try {
            alloc_traits::construct(alloc_, finish_.cur_, *(copy++));
            ++finish_;
            ++size_;
          } catch (...) {
            clear(finish_);
            throw;
          }
        }
      } else {
        finish_ += other.size_;
        size_ = other.size_;
      }
    }
  }

  Deque(size_t count, const T& value, const Alloc& alloc = Alloc())
      : size_(0),
        chunk_(count),
        alloc_(alloc),
        chunk_num_((count + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    for (int64_t i = 0; i < chunk_num_; ++i) {
      try {
        chunk_[i] = alloc_traits::allocate(alloc_, kChunkSize);
      } catch (...) {
        for (int64_t count = 0; count <= i; ++count) {
          alloc_traits::deallocate(alloc_, chunk_[count], kChunkSize);
        }
        throw;
      }
    }
    start_ = iterator(chunk_[0], &centre_, -centre_, &chunk_);
    finish_ = start_;
    while (size_ != count) {
      try {
        alloc_traits::construct(alloc_, finish_.cur_, value);
        ++size_;
        ++finish_;
      } catch (...) {
        clear(finish_);
        throw;
      }
    }
  }

  ~Deque() {
    if (!chunk_.empty()) {
      for (iterator i = start_; i != finish_; ++i) {
        alloc_traits::destroy(alloc_, i.cur_);
      }
      for (int64_t i = start_.ind_; i < finish_.ind_; ++i) {
        alloc_traits::deallocate(alloc_, chunk_[i + centre_], kChunkSize);
        chunk_[i + centre_] = nullptr;
      }
      if (finish_.cur_ - chunk_[(centre_) + finish_.ind_] != 0) {
        alloc_traits::deallocate(alloc_, chunk_[(centre_) + finish_.ind_],
                                 kChunkSize);
        chunk_[(centre_) + finish_.ind_] = nullptr;
      }
    }
  }

  Deque& operator=(const Deque& other) {
    Deque copy = other;
    if (std::allocator_traits<
            Alloc>::propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
    deque_swap(copy);
    return *this;
  }

  Deque(Deque&& other)
      : start_(other.start_),
        finish_(other.finish_),
        chunk_(std::move(other.chunk_)),
        size_(other.size_),
        centre_(other.centre_),
        chunk_num_(other.chunk_num_),
        alloc_(
            alloc_traits::select_on_container_copy_construction(other.alloc_)) {
    start_.chunk_ = &chunk_;
    finish_.chunk_ = &chunk_;
    other.size_ = 0;
    other.chunk_num_ = 0;
    other.centre_ = 0;
    other.start_ = iterator();
    other.finish_ = iterator();
  }

  Deque& operator=(Deque&& other) {
    Deque copy = std::move(other);
    if (std::allocator_traits<
            Alloc>::propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
    deque_swap(copy);
    return *this;
  }

  Deque(std::initializer_list<T> init, const Alloc& alloc = Alloc())
      : size_(init.size()),
        chunk_((init.size() + kChunkSize - 1) / kChunkSize),
        alloc_(alloc),
        chunk_num_((init.size() + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    if (init.size() == 0) {
      chunk_num_ = 0;
      centre_ = 0;
      start_ = iterator();
      finish_ = iterator();
    } else {
      for (int64_t i = 0; i < chunk_num_; ++i) {
        try {
          chunk_[i] = alloc_traits::allocate(alloc_, kChunkSize);
        } catch (...) {
          alloc_traits::deallocate(alloc_, chunk_[i], kChunkSize);
          clear(finish_);
          throw;
        }
      }
      start_ = iterator(chunk_[0], &centre_, -centre_, &chunk_);
      finish_ = start_ + size_;
      auto copy = init.begin();
      if constexpr (std::is_default_constructible_v<T>) {
        for (iterator it = start_; it < finish_; ++it) {
          try {
            alloc_traits::construct(alloc_, it.cur_, *(copy++));
          } catch (...) {
            clear(finish_);
            throw;
          }
        }
      }
    }
  }

  T& operator[](size_t ind) { return *(start_ + ind); }
  const T& operator[](size_t ind) const { return *(start_ + ind); }

  T& at(size_t ind) {
    if (ind >= size_) {
      throw std::out_of_range("...");
    }
    return *(start_ + ind);
  }
  const T& at(size_t ind) const {
    if (ind >= size_) {
      throw std::out_of_range("...");
    }
    return *(start_ + ind);
  }

  iterator begin() { return start_; }

  const_iterator begin() const {
    return const_iterator(start_.cur_, start_.centre_, start_.ind_,
                          start_.chunk_);
  }

  const_iterator cbegin() const {
    return const_iterator(start_.cur_, start_.centre_, start_.ind_,
                          start_.chunk_);
  }

  iterator end() { return finish_; }

  const_iterator end() const {
    return const_iterator((finish_).cur_, (finish_).centre_, (finish_).ind_,
                          (finish_).chunk_);
  }

  const_iterator cend() const {
    return const_iterator((finish_).cur_, (finish_).centre_, (finish_).ind_,
                          (finish_).chunk_);
  }

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

  const allocator_type& get_allocator() const { return alloc_; }

  size_t size() const { return size_; }
  bool empty() { return (size_ == 0); }

  void rememory(const int64_t kResize) {
    std::vector<T*> rechunk(kResize * (size_ + kChunkSize - 1 / kChunkSize));
    int64_t recentre = rechunk.size() / 2;
    if (size_ != 0) {
      for (int64_t ind = start_.ind_; ind <= finish_.ind_; ++ind) {
        rechunk[recentre + ind] = chunk_[centre_ + ind];
      }
    }
    chunk_ = std::move(rechunk);
    centre_ = recentre;
    chunk_num_ *= kResize;
  }

  void push_back(const T& value) {
    try {
      if (!(chunk_.empty())) {
        bool balloon =
            ((finish_.cur_ - (chunk_)[finish_.ind_ + *(finish_.centre_)]) ==
             kChunkSize - 1);
        if (balloon) {
          if (finish_.ind_ == centre_) {
            const int64_t kResize = 51;
            rememory(kResize);
          }
          (chunk_)[centre_ + finish_.ind_ + 1] =
              alloc_traits::allocate(alloc_, kChunkSize);
        }
        alloc_traits::construct(alloc_, finish_.cur_, value);
        ++size_;
        ++finish_;
      } else {
        Deque copy(1, value);
        *this = std::move(copy);
        return;
      }
    } catch (...) {
      throw;
    }
  }

  void push_back(T&& value) {
    try {
      if (!(chunk_.empty())) {
        bool balloon =
            (finish_.cur_ - (chunk_)[finish_.ind_ + *(finish_.centre_)]) ==
            kChunkSize - 1;
        if (balloon) {
          if (finish_.ind_ == centre_) {
            const int64_t kResize = 51;
            rememory(kResize);
          }
          (chunk_)[centre_ + finish_.ind_ + 1] =
              alloc_traits::allocate(alloc_, kChunkSize);
        }
        alloc_traits::construct(alloc_, finish_.cur_, std::move(value));
        ++size_;
        ++finish_;
      } else {
        Deque tmp(1);
        alloc_traits::destroy(alloc_, tmp.start_.cur_);
        *this = std::move(tmp);
        alloc_traits::construct(alloc_, start_.cur_, std::move(value));
      }
    } catch (...) {
      throw;
    }
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    try {
      if (!(chunk_.empty())) {
        bool balloon =
            (finish_.cur_ - (chunk_)[finish_.ind_ + *(finish_.centre_)]) ==
            kChunkSize - 1;
        if (balloon) {
          if (finish_.ind_ == centre_) {
            const int64_t kResize = 50;
            rememory(kResize);
          }
          (chunk_)[centre_ + finish_.ind_ + 1] =
              alloc_traits::allocate(alloc_, kChunkSize);
        }
        alloc_traits::construct(alloc_, finish_.cur_,
                                std::forward<Args>(args)...);
        ++size_;
        ++finish_;
      } else {
        Deque tmp(1);
        alloc_traits::destroy(alloc_, tmp.start_.cur_);
        *this = std::move(tmp);
        alloc_traits::construct(alloc_, start_.cur_,
                                std::forward<Args>(args)...);
      }
    } catch (...) {
      throw;
    }
  }
  void pop_back() {
    --finish_;
    alloc_traits::destroy(alloc_, finish_.cur_);
    if (finish_.cur_ - chunk_[centre_ + finish_.ind_] == kChunkSize - 1) {
      alloc_traits::destroy(alloc_, chunk_[centre_ + finish_.ind_ + 1]);
      alloc_traits::deallocate(alloc_, chunk_[(centre_) + finish_.ind_ + 1],
                               kChunkSize);
    }
    --size_;
  }

  void erase(iterator iter) {
    alloc_traits::destroy(alloc_, iter.cur_);
    for (iterator i = iter; i < finish_ - 1; ++i) {
      try {
        *i.cur_ = *((i + 1).cur_);
      } catch (...) {
        throw;
      }
    }
    this->pop_back();
  }

  void insert(iterator iter, const T& value) {
    if (iter == finish_) {
      push_back(value);
      return;
    }
    bool balloon =
        (finish_.cur_ == chunk_[centre_ + finish_.ind_] + kChunkSize - 1);
    if ((finish_.ind_ + centre_ == chunk_num_ - 1)) {
      const int64_t kResize = 50;
      rememory(kResize);
    }
    if (balloon) {
      chunk_[finish_.ind_ + centre_ + 1] =
          alloc_traits::allocate(alloc_, kChunkSize);
    }
    iterator tmp_iter = finish_;
    for (; tmp_iter > iter; --tmp_iter) {
      *tmp_iter = *(tmp_iter - 1);
    }
    *tmp_iter = value;
    ++size_;
    ++finish_;
  }

  void emplace(iterator iter, T&& value) {
    if (chunk_.empty() == 0) {
      Deque<T> tmp(1, std::forward<T>(value));
      *this = tmp;
      return;
    }
    bool balloon =
        (finish_.cur_ == chunk_[centre_ + finish_.ind_] + kChunkSize - 1);
    if ((finish_.ind_ + centre_ == chunk_num_ - 1)) {
      const int64_t kResize = 50;
      rememory(kResize);
    }
    if (balloon) {
      chunk_[finish_.ind_ + centre_ + 1] =
          alloc_traits::allocate(alloc_, kChunkSize);
    }
    iterator tmp_iter = finish_;
    for (; tmp_iter > iter; --tmp_iter) {
      *tmp_iter = *(tmp_iter - 1);
    }
    *tmp_iter = (std::forward<T>(value));
    ++size_;
    ++finish_;
  }

  void push_front(const T& value) {
    try {
      if (!(chunk_.empty())) {
        bool balloon =
            (start_.cur_ - (chunk_)[start_.ind_ + *(start_.centre_)]) == 0;
        if (balloon) {
          if (start_.ind_ == -centre_) {
            const int64_t kResize = 50;
            rememory(kResize);
          }
          (chunk_)[centre_ + start_.ind_ - 1] =
              alloc_traits::allocate(alloc_, kChunkSize);
        }
        --start_;
        alloc_traits::construct(alloc_, start_.cur_, value);
        ++size_;
      } else {
        Deque copy(1, value);
        *this = std::move(copy);
        return;
      }
    } catch (...) {
      throw;
    }
  }

  void push_front(T&& value) {
    try {
      if (!(chunk_.empty())) {
        bool balloon =
            (start_.cur_ - (chunk_)[start_.ind_ + *(start_.centre_)]) == 0;
        if (balloon) {
          if (start_.ind_ == -centre_) {
            const int64_t kResize = 50;
            rememory(kResize);
          }
          (chunk_)[centre_ + start_.ind_ - 1] =
              alloc_traits::allocate(alloc_, kChunkSize);
        }
        --start_;
        alloc_traits::construct(alloc_, start_.cur_, std::move(value));
        ++size_;
      } else {
        Deque tmp(1);
        alloc_traits::destroy(alloc_, tmp.start_.cur_);
        *this = std::move(tmp);
        alloc_traits::construct(alloc_, start_.cur_, std::move(value));
      }
    } catch (...) {
      throw;
    }
  }

  template <typename... Args>
  void emplace_front(Args&&... args) {
    try {
      if (!(chunk_.empty())) {
        bool balloon =
            (start_.cur_ - (chunk_)[start_.ind_ + *(start_.centre_)]) == 0;
        if (balloon) {
          if (start_.ind_ == -centre_) {
            const int64_t kResize = 50;
            rememory(kResize);
          }
          (chunk_)[centre_ + start_.ind_ - 1] =
              alloc_traits::allocate(alloc_, kChunkSize);
        }
        --start_;
        alloc_traits::construct(alloc_, start_.cur_,
                                std::forward<Args>(args)...);
        ++size_;
      } else {
        Deque tmp(1);
        alloc_traits::destroy(alloc_, tmp.start_.cur_);
        *this = std::move(tmp);
        alloc_traits::construct(alloc_, start_.cur_,
                                std::forward<Args>(args)...);
      }
    } catch (...) {
      throw;
    }
  }
  void pop_front() {
    alloc_traits::destroy(alloc_, start_.cur_);
    ++start_;
    if (start_.cur_ == chunk_[centre_ + start_.ind_]) {
      alloc_traits::destroy(alloc_, chunk_[centre_ + start_.ind_ - 1]);
      alloc_traits::deallocate(alloc_, chunk_[(centre_) + start_.ind_ - 1],
                               kChunkSize);
    }
    --size_;
  }

 private:
  size_t size_ = 0;
  std::vector<T*> chunk_;
  Alloc alloc_;
  int64_t chunk_num_ = 0;
  int64_t centre_ = 0;
  iterator start_ = iterator();
  iterator finish_ = iterator();
  static const int64_t kChunkSize = 24;

  void def_deque() {
    chunk_num_ = 0;
    centre_ = 0;
    start_ = iterator();
    finish_ = iterator();
  }

  void deque_swap(Deque& copy) {
    std::swap(size_, copy.size_);
    std::swap(chunk_, copy.chunk_);
    std::swap(start_, copy.start_);
    std::swap(chunk_num_, copy.chunk_num_);
    std::swap(centre_, copy.centre_);
    std::swap(finish_, copy.finish_);
    std::swap(start_.chunk_, copy.start_.chunk_);
    std::swap(finish_.chunk_, copy.finish_.chunk_);
    start_.centre_ = &centre_;
    finish_.centre_ = &centre_;
    if (copy.chunk_.empty()) {
      start_.chunk_ = &chunk_;
      finish_.chunk_ = &chunk_;
    }
  }
  void clear(iterator iter) {
    if (!chunk_.empty()) {
      for (iterator i = start_; i < iter; ++i) {
        alloc_traits::destroy(alloc_, i.cur_);
      }
      for (int64_t i = start_.ind_; i < chunk_num_; ++i) {
        alloc_traits::deallocate(alloc_, chunk_[i + centre_], kChunkSize);
        chunk_[i + centre_] = nullptr;
      }
      if (iter.cur_ - chunk_[(centre_) + iter.ind_] != 0) {
        alloc_traits::deallocate(alloc_, chunk_[(centre_) + iter.ind_],
                                 kChunkSize);
        chunk_[(centre_) + iter.ind_] = nullptr;
      }
    }
  }
};

template <typename T, typename Alloc>
template <bool IsConst>
class Deque<T, Alloc>::common_iterator {
 public:
  using type = std::conditional_t<IsConst, const T, T>;
  using value_type = T;
  using difference_type = int64_t;
  using pointer = type*;
  using reference = type&;
  using iterator_category = std::random_access_iterator_tag;

  common_iterator() = default;
  common_iterator(T* cur, int64_t* centre, int64_t ind, std::vector<T*>* chunk)
      : cur_(cur), centre_(centre), ind_(ind), chunk_(chunk) {}
  common_iterator(const common_iterator& other)
      : cur_(other.cur_),
        centre_(other.centre_),
        ind_(other.ind_),
        chunk_(other.chunk_) {}

  type& operator*() { return *cur_; }
  const type& operator*() const { return *cur_; }

  type* operator->() { return cur_; }
  const type* operator->() const { return cur_; }

  common_iterator<IsConst>& operator++() {
    if (cur_ + 1 == (*chunk_)[*centre_ + ind_] + kChunkSize) {
      ind_ += 1;
      cur_ = (*chunk_)[*centre_ + ind_];
    } else {
      cur_++;
    }
    return *this;
  }
  common_iterator<IsConst>& operator--() {
    if (cur_ == (*chunk_)[*centre_ + ind_]) {
      ind_ -= 1;
      cur_ = (*chunk_)[*centre_ + ind_] + kChunkSize - 1;
    } else {
      cur_--;
    }
    return *this;
  }

  bool operator==(const common_iterator& other) const {
    return ((*this - other) == 0);
  }
  bool operator!=(const common_iterator& other) const {
    return !(*this == other);
  }
  bool operator<(const common_iterator& other) const {
    return (*this - other < 0);
  }
  bool operator>(const common_iterator& other) const { return (other < *this); }
  bool operator<=(const common_iterator& other) const {
    return !(other > *this);
  }
  bool operator>=(const common_iterator& other) const {
    return !(other < *this);
  }
  common_iterator<IsConst> operator++(int) {
    common_iterator tmp = *this;
    ++(*this);
    return tmp;
  }
  common_iterator<IsConst> operator--(int) {
    common_iterator tmp = *this;
    --(*this);
    return tmp;
  }
  common_iterator<IsConst>& operator+=(difference_type num) {
    if (num == 0) {
      return *this;
    }
    int64_t shift = num + (cur_ - (*chunk_)[*centre_ + ind_]);
    if (shift >= 0 && shift < kChunkSize) {
      cur_ += num;
    } else {
      int64_t chunk_shift;
      if (shift > 0) {
        chunk_shift = shift / kChunkSize;
      } else {
        chunk_shift = ((shift + 1) / kChunkSize) - 1;
      }
      ind_ += chunk_shift;
      cur_ = (*chunk_)[*centre_ + ind_] + (shift - (chunk_shift * kChunkSize));
    }
    return *this;
  }
  common_iterator<IsConst>& operator-=(const int64_t& num) {
    return *this += -num;
  }
  common_iterator<IsConst> operator+(const int64_t& num) const {
    common_iterator tmp = *this;
    tmp += num;
    return tmp;
  }
  common_iterator<IsConst> operator-(const int64_t& num) const {
    common_iterator tmp = *this;
    tmp -= num;
    return tmp;
  }
  int64_t operator-(const common_iterator& other) const {
    if (cur_ == nullptr && other.cur_ == nullptr) {
      return 0;
    }
    return (ind_ - other.ind_) * kChunkSize +
           (cur_ - (*chunk_)[*centre_ + ind_]) +
           (((*other.chunk_)[*(other.centre_) + other.ind_]) - other.cur_);
  }

  common_iterator& operator=(const common_iterator& other) {
    cur_ = other.cur_;
    centre_ = other.centre_;
    ind_ = other.ind_;
    chunk_ = other.chunk_;
    return *this;
  }

  friend class Deque;

 private:
  T* cur_;
  int64_t* centre_;
  int64_t ind_;
  std::vector<T*>* chunk_;
};
