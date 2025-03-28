#pragma once

#include <cassert>
#include <cstddef>
#include <memory>

namespace {

enum class BlockType {
  Simple,
  Deleter,
  Data,
  Allocator,
  DeleterAndAllocator,
};

struct Block {
  Block(BlockType type) : kType(type) {}

  virtual ~Block() = default;

  size_t counter = 1;
  size_t weak_counter = 0;
  const BlockType kType;
};

struct BlockSimpleBase : Block {
  BlockSimpleBase() : Block(BlockType::Simple) {}

  virtual void free() noexcept = 0;

  virtual ~BlockSimpleBase() = default;
};

template <typename T>
struct BlockSimple final : BlockSimpleBase {
  BlockSimple(T* ptr) : BlockSimpleBase(), data(ptr) {}

  virtual void free() noexcept override { delete data; }

  T* data;
};

struct BlockDelBase : Block {
  BlockDelBase() : Block(BlockType::Deleter) {}

  virtual void free() noexcept = 0;

  virtual ~BlockDelBase() = default;
};

template <typename T, typename Deleter>
struct BlockDel final : BlockDelBase {
  BlockDel(T* ptr, const Deleter& deleter_ref)
      : BlockDelBase(), deleter(deleter_ref), data(ptr) {}

  virtual void free() noexcept override { deleter(data); }

  Deleter deleter;
  T* data;
};

struct BlockDelAllocBase : Block {
  BlockDelAllocBase() : Block(BlockType::DeleterAndAllocator) {}

  virtual void free() noexcept = 0;

  virtual void deallocate() noexcept = 0;

  virtual ~BlockDelAllocBase() = default;
};

template <typename T, typename Deleter, typename Allocator>
struct BlockDelAlloc final : BlockDelAllocBase {
  using base_alloc_traits = std::allocator_traits<Allocator>;
  using block_alloc_t =
      typename base_alloc_traits::template rebind_alloc<BlockDelAlloc>;
  using block_alloc_traits = std::allocator_traits<block_alloc_t>;

  BlockDelAlloc(T* ptr, const Deleter& deleter_ref,
                const block_alloc_t& alloc_ref)
      : deleter(deleter_ref), data(ptr), alloc(alloc_ref) {}

  virtual void free() noexcept override { deleter(data); }

  virtual void deallocate() noexcept override {
    block_alloc_t alloc_copy(alloc);

    block_alloc_traits::destroy(alloc_copy, this);
    block_alloc_traits::deallocate(alloc_copy, this, 1);
  }

  Deleter deleter;
  T* data;
  block_alloc_t alloc;
};

struct BlockDataBase : Block {
  BlockDataBase() : Block(BlockType::Data) {}

  virtual void destroy() noexcept = 0;
};

template <typename T>
struct BlockData final : BlockDataBase {
  template <typename... Args>
  BlockData(Args&&... args) {
    ::new (&data) T(std::forward<Args>(args)...);
  }

  virtual void destroy() noexcept override {
    reinterpret_cast<T*>(&data)->~T();
  }

  std::aligned_storage_t<sizeof(T), alignof(T)> data;
};

struct BlockAllocBase : Block {
  BlockAllocBase() : Block(BlockType::Allocator) {}

  virtual void destroy() noexcept = 0;

  virtual void deallocate() noexcept = 0;

  virtual ~BlockAllocBase() = default;
};

template <typename T, typename Allocator>
struct BlockAlloc final : BlockAllocBase {
  using base_alloc_traits = std::allocator_traits<Allocator>;
  using block_alloc_t =
      typename base_alloc_traits::template rebind_alloc<BlockAlloc>;
  using block_alloc_traits = std::allocator_traits<block_alloc_t>;

  template <typename... Args>
  BlockAlloc(const block_alloc_t& alloc_ref, Args&&... args)
      : BlockAllocBase(), alloc(alloc_ref) {
    ::new (&data) T(std::forward<Args>(args)...);
  }

  virtual void destroy() noexcept override {
    reinterpret_cast<T*>(&data)->~T();
  }

  virtual void deallocate() noexcept override {
    block_alloc_t alloc_copy(alloc);

    block_alloc_traits::destroy(alloc_copy, this);
    block_alloc_traits::deallocate(alloc_copy, this, 1);
  }

  std::aligned_storage_t<sizeof(T), alignof(T)> data;
  block_alloc_t alloc;
};

}  
template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
 private:
  friend WeakPtr<T>;

  template <typename U, typename... Args>
  friend SharedPtr<U> MakeShared(Args&&... args);

  template <typename U, typename Allocator, typename... Args>
  friend SharedPtr<U> AllocateShared(const Allocator& alloc, Args&&... args);

  template <typename U>
  friend class SharedPtr;

 public:
  SharedPtr() : data_(nullptr), block_(nullptr) {}

  SharedPtr(std::nullptr_t) : data_(nullptr), block_(nullptr){};

  template <typename Y>
  explicit SharedPtr(Y* data)
      : data_(data), block_(new ::BlockSimple<Y>(data)) {}

  template <typename Y, typename Deleter>
  explicit SharedPtr(Y* data, const Deleter& deleter)
      : data_(data), block_(new ::BlockDel<Y, Deleter>(data, deleter)) {}

  template <typename Y, typename Deleter, typename Allocator>
  explicit SharedPtr(Y* data, const Deleter& deleter, Allocator& alloc)
      : data_(data), block_(nullptr) {
    using block_t = ::BlockDelAlloc<Y, Deleter, Allocator>;
    using block_alloc_t = typename std::allocator_traits<
        Allocator>::template rebind_alloc<block_t>;
    using block_alloc_traits = std::allocator_traits<block_alloc_t>;

    block_alloc_t block_alloc(alloc);

    auto ptr = block_alloc_traits::allocate(block_alloc, 1);
    block_alloc_traits::construct(block_alloc, ptr, data, deleter, block_alloc);

    block_ = ptr;
  }

  SharedPtr(const SharedPtr& other) : data_(other.data_), block_(other.block_) {
    add_ref();
  }

  template <class Y>
  SharedPtr(const SharedPtr<Y>& other)
      : data_(other.data_), block_(other.block_) {
    add_ref();
  }

  SharedPtr& operator=(const SharedPtr& other) {
    if (&other == this) {
      return *this;
    }

    remove_ref();

    data_ = other.data_;
    block_ = other.block_;

    add_ref();

    return *this;
  }

  template <class Y>
  SharedPtr<T>& operator=(const SharedPtr<Y>& other) {
    remove_ref();

    data_ = other.data_;
    block_ = other.block_;

    add_ref();

    return *this;
  }

  SharedPtr(SharedPtr&& other) : data_(other.data_), block_(other.block_) {
    other.data_ = nullptr;
    other.block_ = nullptr;
  }

  template <class Y>
  SharedPtr(SharedPtr<Y>&& other) : data_(other.data_), block_(other.block_) {
    other.data_ = nullptr;
    other.block_ = nullptr;
  }

  SharedPtr<T>& operator=(SharedPtr&& other) {
    if (&other == this) {
      return *this;
    }

    remove_ref();

    data_ = other.data_;
    block_ = other.block_;

    other.data_ = nullptr;
    other.block_ = nullptr;

    return *this;
  }

  template <class Y>
  SharedPtr<T>& operator=(SharedPtr<Y>&& other) {
    remove_ref();

    data_ = other.data_;
    block_ = other.block_;

    other.data_ = nullptr;
    other.block_ = nullptr;

    return *this;
  }

  T& operator*() noexcept { return *data_; }

  const T& operator*() const noexcept { return *data_; }

  T* operator->() noexcept { return data_; }

  const T* operator->() const noexcept { return data_; }

  T* get() const noexcept { return data_; }

  size_t use_count() const { return (block_ != nullptr) ? block_->counter : 0; }

  void reset() noexcept {
    remove_ref();

    data_ = nullptr;
    block_ = nullptr;
  }

  ~SharedPtr() {
    if (data_ != nullptr) {
      remove_ref();
    }
  }

 private:
  SharedPtr(T* data, ::Block* block, int)
      : data_(data), block_(block) {}

  void add_ref() {
    if (block_ != nullptr) {
      block_->counter++;
    }
  }

  void remove_ref() {
    if (block_ == nullptr) {
      return;
    }

    block_->counter--;

    if (block_->counter == 0) {
      delete_data();
    }

    data_ = nullptr, block_ = nullptr;
  }

  void delete_data() {
    switch (block_->kType) {
      case ::BlockType::Simple:
        static_cast<::BlockSimpleBase*>(block_)->free();
        break;
      case ::BlockType::Deleter:
        static_cast<::BlockDelBase*>(block_)->free();
        break;
      case ::BlockType::Data:
        static_cast<::BlockDataBase*>(block_)->destroy();
        break;
      case ::BlockType::Allocator:
        static_cast<::BlockAllocBase*>(block_)->destroy();
        if (block_->weak_counter == 0) {
          static_cast<::BlockAllocBase*>(block_)->deallocate();
        }
        return;
      case ::BlockType::DeleterAndAllocator:
        static_cast<::BlockDelAllocBase*>(block_)->free();
        if (block_->weak_counter == 0) {
          static_cast<::BlockDelAllocBase*>(block_)->deallocate();
        }
        return;
      default:
        assert(false);
    }

    check_weak_counter();
  }

  void check_weak_counter() {
    if (block_->weak_counter == 0) {
      delete block_;
    }
  }

  T* data_;
  ::Block* block_;
};


template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  auto ptr = new ::BlockData<T>(std::forward<Args>(args)...);
  auto data = reinterpret_cast<T*>(&ptr->data);
  return SharedPtr<T>(data, ptr, 0);
}

template <typename T, typename Allocator, typename... Args>
SharedPtr<T> AllocateShared(const Allocator& alloc, Args&&... args) {
  using block_t = ::BlockAlloc<T, Allocator>;
  using block_alloc_t = typename block_t::block_alloc_t;
  using traits_t = typename block_t::block_alloc_traits;

  block_alloc_t block_alloc(alloc);

  auto ptr = traits_t::allocate(block_alloc, 1);
  traits_t::construct(block_alloc, ptr, block_alloc,
                      std::forward<Args>(args)...);

  auto data = reinterpret_cast<T*>(&ptr->data);

  return SharedPtr<T>(data, ptr, 0);
}


template <typename T>
class WeakPtr {
 public:
  WeakPtr() : data_(nullptr), block_(nullptr) {}

  WeakPtr(const SharedPtr<T>& ptr) : data_(ptr.data_), block_(ptr.block_) {
    add_weak_ref();
  }

  WeakPtr(const WeakPtr<T>& ptr) : data_(ptr.data_), block_(ptr.block_) {
    add_weak_ref();
  }

  WeakPtr& operator=(const WeakPtr& other) {
    if (&other == this) {
      return *this;
    }

    remove_weak_ref();

    data_ = other.data_;
    block_ = other.block_;

    add_weak_ref();

    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) {
    if (&other == this) {
      return *this;
    }

    remove_weak_ref();

    data_ = other.data_;
    block_ = other.block_;

    other.data_ = nullptr;
    other.block_ = nullptr;

    return *this;
  }

  SharedPtr<T> lock() {
    if (data_ != nullptr) {
      return {data_, block_};
    }
    return {nullptr};
  }

  bool expired() const { return (block_->counter == 0); }

  ~WeakPtr() { remove_weak_ref(); }

 private:
  void add_weak_ref() {
    if (block_ != nullptr) {
      block_->weak_counter++;
    }
  }

  void remove_weak_ref() {
    if (block_ != nullptr) {
      block_->weak_counter--;

      if (block_->weak_counter == 0) {
        if (block_->kType == ::BlockType::Allocator) {
          static_cast<::BlockAllocBase*>(block_)->deallocate();
        } else if (block_->kType == ::BlockType::DeleterAndAllocator) {
          static_cast<::BlockDelAllocBase*>(block_)->deallocate();
        } else {
          delete block_;
        }
      }
    }
  }

  T* data_;
  ::Block* block_;
};
