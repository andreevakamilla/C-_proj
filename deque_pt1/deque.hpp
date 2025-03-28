#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <type_traits>

template <typename T>
class Deque {
 public:
  template <bool IsConst>
  class common_iterator;

  using iterator = common_iterator<false>;
  using const_iterator = common_iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  Deque() {}

  Deque(size_t count)
      : size_(0),
        chunk_((count + kChunkSize - 1) / kChunkSize),
        chunk_num_((count + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    fill_chunk();
    start_ = iterator((chunk_)[0], &centre_, -centre_, &chunk_);
    finish_ = start_;
    if constexpr (std::is_default_constructible_v<T>) {
      while (size_ != count) {
        try {
          new (finish_.cur_) T();
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
        chunk_num_((other.size_ + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    if (other.size_ == 0) {
      deque_def();
    } else {
      fill_chunk();
      start_ = iterator((chunk_)[0], &centre_, -centre_, &chunk_);
      finish_ = start_;
      iterator copy = other.start_;
      if constexpr (std::is_default_constructible_v<T>) {
        while (size_ != other.size_) {
          try {
            new (finish_.cur_) T(*(copy++));
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

  Deque(size_t count, const T& value)
      : size_(0),
        chunk_(count),
        chunk_num_((count + kChunkSize - 1) / kChunkSize),
        centre_(chunk_num_ / 2) {
    fill_chunk();
    start_ = iterator((chunk_)[0], &centre_, -centre_, &chunk_);
    finish_ = start_;
    while (size_ != count) {
      try {
        new (finish_.cur_) T(value);
        ++size_;
        ++finish_;
      } catch (...) {
        clear(finish_);
        throw;
      }
    }
  }

  ~Deque() {
    if (!(chunk_.empty())) {
      for (iterator i = start_; i != finish_; ++i) {
        i.cur_->~T();
      }
      for (int64_t i = start_.ind_; i < finish_.ind_; ++i) {
        delete[](chunk_)[i + centre_];
        (chunk_)[i + centre_] = nullptr;
      }
      if (finish_.cur_ - (chunk_)[(centre_) + finish_.ind_] != 0) {
        delete[](chunk_)[(centre_) + finish_.ind_];
        (chunk_)[(centre_) + finish_.ind_] = nullptr;
      }
    }
  }

  Deque& operator=(const Deque& other) {
    if (this != &other) {
      Deque copy = other;
      deque_swap(copy);
    }
    return *this;
  }

  Deque(Deque&& other)
      : start_(other.start_),
        finish_(other.finish_),
        chunk_(std::move(other.chunk_)),
        size_(other.size_),
        centre_(other.centre_),
        chunk_num_(other.chunk_num_) {
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
    deque_swap(copy);
    return *this;
  }

  T& operator[](size_t ind) { return *(start_ + ind); }
  const T& operator[](size_t ind) const { return *(start_ + ind); }

  T& at(size_t ind) {
    if (ind >= size_) {
      throw std::out_of_range("нормальная ошибка");
    }
    return *(start_ + ind);
  }
  const T& at(size_t ind) const {
    if (ind >= size_) {
      throw std::out_of_range("нормальная ошибка");
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
    if (!empty()) {
      bool balloon =
          (finish_.cur_ - (chunk_)[finish_.ind_ + *(finish_.centre_)]) ==
          kChunkSize - 1;
      if (balloon) {
        if (finish_.ind_ == centre_) {
          const int64_t kResize = 51;
          rememory(kResize);
        }
        (chunk_)[finish_.ind_ + centre_ + 1] =
            reinterpret_cast<T*>(new uint8_t[kChunkSize * sizeof(T)]);
      }
      new (finish_.cur_) T(value);
      ++size_;
      ++finish_;
    } else {
      Deque copy(1, value);
      *this = std::move(copy);
      return;
    }
  }

  void push_back(T&& value) {
    if (!empty()) {
      bool balloon =
          (finish_.cur_ - (chunk_)[finish_.ind_ + *(finish_.centre_)]) ==
          kChunkSize - 1;
      if (balloon) {
        if (finish_.ind_ == centre_) {
          const int64_t kResize = 51;
          rememory(kResize);
        }
        (chunk_)[finish_.ind_ + centre_ + 1] =
            reinterpret_cast<T*>(new uint8_t[kChunkSize * sizeof(T)]);
      }
      new (finish_.cur_) T(std::move(value));
      ++size_;
      ++finish_;
    } else {
      Deque copy(1);
      copy.start_.cur_->~T();
      *this = std::move(copy);
      new (start_.cur_) T(std::move(value));
      return;
    }
  }

  void pop_back() {
    --finish_;
    finish_.cur_->~T();
    if (finish_.cur_ - (chunk_)[(centre_) + finish_.ind_] == 0) {
      delete[](chunk_)[(centre_) + finish_.ind_];
    }
    --size_;
  }

  void erase(iterator iter) {
    iter->~T();
    for (iterator i = iter; i < finish_ - 1; ++i) {
      *i.cur_ = *((i + 1).cur_);
    }
    this->pop_back();
  }

  void insert(iterator iter, const T& value) {
    if (iter == finish_) {
      push_back(value);
      return;
    }
    bool balloon =
        (finish_.cur_ == (chunk_)[centre_ + finish_.ind_] + kChunkSize - 1);
    if ((finish_.ind_ + centre_ == chunk_num_ - 1)) {
      const int64_t kResize = 50;
      rememory(kResize);
    }
    if (balloon) {
      (chunk_)[finish_.ind_ + centre_ + 1] =
          reinterpret_cast<T*>(new uint8_t[kChunkSize * sizeof(T)]);
    }
    iterator tmp_iter = finish_;
    for (; tmp_iter > iter; --tmp_iter) {
      *tmp_iter = *(tmp_iter - 1);
    }
    *tmp_iter = value;
    ++size_;
    ++finish_;
  }

  void push_front(const T& value) {
    bool balloon = (start_.cur_ == (chunk_)[centre_ + start_.ind_]);
    if (!empty()) {
      if (balloon && (start_.ind_ == -(centre_))) {
        const int64_t kResize = 50;
        rememory(kResize);
        (chunk_)[start_.ind_ + centre_ - 1] =
            reinterpret_cast<T*>(new uint8_t[kChunkSize * sizeof(T)]);
        --start_;
        new (start_.cur_) T(value);
      } else if (balloon) {
        (chunk_)[start_.ind_ + centre_ - 1] =
            reinterpret_cast<T*>(new uint8_t[kChunkSize * sizeof(T)]);
        add_front(value);
      } else {
        add_front(value);
      }
      ++size_;
    } else {
      Deque<T> tmp(1, value);
      *this = tmp;
      return;
    }
  }

  void pop_front() {
    start_->~T();
    ++start_;
    if (start_.cur_ == (chunk_)[centre_ + start_.ind_]) {
      delete[](chunk_)[centre_ + start_.ind_ - 1];
    }
    --size_;
  }

 private:
  size_t size_ = 0;
  std::vector<T*> chunk_;
  int64_t chunk_num_ = 0;
  int64_t centre_ = 0;
  iterator start_ = iterator();
  iterator finish_ = iterator();
  static const int64_t kChunkSize = 24;

  void clear(iterator iter) {
    if (!(chunk_.empty())) {
      for (iterator i = start_; i < finish_; ++i) {
        i.cur_->~T();
      }
      for (int64_t i = start_.ind_; i < chunk_num_; ++i) {
        delete[](chunk_)[i + centre_];
        (chunk_)[i + centre_] = nullptr;
      }
      if (iter.cur_ - (chunk_)[(centre_) + iter.ind_] != 0) {
        delete[](chunk_)[(centre_) + iter.ind_];
        chunk_[(centre_) + iter.ind_] = nullptr;
      }
    }
  }

  void fill_chunk() {
    for (int64_t ind = 0; ind < chunk_num_; ++ind) {
      recast(ind);
    }
  }

  void recast(int64_t ind) {
    (chunk_)[ind] = reinterpret_cast<T*>(new uint8_t[kChunkSize * sizeof(T)]);
  }

  void deque_def() {
    chunk_num_ = 0;
    centre_ = 0;
    start_ = iterator();
    finish_ = iterator();
  }

  void add_front(const T& value) {
    --start_;
    new (start_.cur_) T(value);
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
};

template <typename T>
template <bool IsConst>
class Deque<T>::common_iterator {
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
