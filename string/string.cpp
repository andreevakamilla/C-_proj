#include "string.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

String::String(int size, char letter) : size_(size), capacity_(size_) {
  string_ = new char[size_ + 1];
  for (int i = 0; i < size; i++) {
    string_[i] = letter;
  }
  Zero();
}
String::String(const char* cstr) : size_(strlen(cstr)), capacity_(size_) {
  string_ = new char[size_ + 1];
  memcpy(string_, cstr, size_);
  Zero();
}
String::String(const String& other) {
  size_ = strlen(other.string_);
  capacity_ = size_;
  string_ = new char[size_ + 1];
  for (int i = 0; i < size_; i++) {
    string_[i] = other.string_[i];
  }
  Zero();
}
String& String::operator=(const String& other) {
  if (this == &other) {
    return *this;
  }
  size_ = other.size_;
  capacity_ = other.capacity_;
  delete[] string_;
  string_ = new char[capacity_ + 1];
  if (other.string_ != nullptr) {
    memcpy(string_, other.string_, size_);
  }
  Zero();
  return *this;
}

size_t String::Size() const { return size_; }
size_t String::Capacity() const { return capacity_; }
bool String::Empty() const { return (size_ == 0); }
void String::Clear() {
  if (!(this->Empty())) {
    size_ = 0;
    string_[0] = '\0';
  }
}
void String::PushBack(const char& character) {
  if (size_ == capacity_) {
    Reserve(2 * size_ + 1);
  }
  string_[size_] = character;
  ++size_;
  Zero();
}
void String::PopBack() {
  if (size_ > 0) {
    --size_;
    Zero();
  }
}
void String::Resize(int new_size) {
  if (new_size > capacity_) {
    char* new_str = new char[new_size + 1];
    for (int i = 0; i < size_; i++) {
      new_str[i] = string_[i];
    }
    delete[] string_;
    string_ = new_str;
  }
  size_ = new_size;
  Zero();
}
void String::Resize(int new_size, char character) {
  int size = size_;
  Resize(new_size);
  if (size < new_size) {
    memset(string_ + size, character, new_size - size);
  }
  string_[new_size] = '\0';
}
void String::Reserve(int new_cap) {
  if (new_cap > capacity_) {
    char* copy_string = new char[new_cap + 1];
    if (string_ != nullptr) {
      memcpy(copy_string, string_, size_);
      delete[] string_;
    }
    capacity_ = new_cap;
    string_ = copy_string;
    Zero();
  }
}
void String::ShrinkToFit() {
  if (capacity_ > size_) {
    capacity_ = size_;
  }
}
void String::Swap(String& other) {
  char* ttt;
  ttt = other.string_;
  other.string_ = string_;
  string_ = ttt;
}
char& String::operator[](size_t iii) { return string_[iii]; }
const char& String::operator[](size_t iii) const { return string_[iii]; }
char& String::Front() { return this->string_[0]; }
char& String::Back() { return this->string_[size_ - 1]; }
char String::Front() const { return this->string_[0]; }
char String::Back() const { return this->string_[size_ - 1]; }
const char* String::Data() const { return string_; }
char* String::Data() { return string_; }
bool String::operator<(String other) const {
  if ((string_ == nullptr) || (other.string_ == nullptr)) {
    return false;
  }
  if (this->size_ < other.size_) {
    return true;
  }
  for (int i = 0; i < size_; i++) {
    if (string_[i] != other.string_[i]) {
      return string_[i] < other.string_[i];
    }
  }
  return false;
}
bool String::operator>(String other) const { return (other < *this); }
bool String::operator<=(String other) const { return !(*this > other); }
bool String::operator>=(String other) const { return !(*this < other); }
bool String::operator==(const String& other) const {
  return (!(*this < other) && !(*this > other));
}
bool String::operator!=(String other) const {
  return !(this->string_ == other.string_);
}
char& String::operator[](int index) { return this->string_[index]; }
const char& String::operator[](int index) const { return this->string_[index]; }

String& String::operator+=(const String& other) {
  if (capacity_ < size_ + other.size_) {
    Reserve(2 * (size_ + other.size_));
  }
  if (string_ != nullptr) {
    memcpy(string_ + size_, other.string_, other.size_);
    size_ += other.size_;
    Zero();
  }
  return *this;
}
String operator+(const String& first_string, const String& second_string) {
  String result = "";
  result += first_string;
  result += second_string;
  return result;
}
String operator*(const String& string, int n) {
  String result = string;
  result *= n;
  return result;
}

String& String::operator*=(int n) {
  if (capacity_ <= n * size_) {
    Reserve(2 * n * size_ + 1);
  }
  for (int i = 0; i < n * size_; i++) {
    string_[i] = string_[i % size_];
  }
  size_ *= n;
  if (string_ != nullptr) {
    Zero();
  }
  return *this;
}
std::istream& operator>>(std::istream& iis, String& other) {
  other.Clear();
  char character;
  while ((iis.get(character)) && !(iis.eof())) {
    other.PushBack(character);
  }
  return iis;
}
std::ostream& operator<<(std::ostream& oos, const String& other) {
  oos << other.string_;
  return oos;
}
std::vector<String> String::Split(const String& delim) {
  std::vector<String> result;
  String part = "";
  int counter = 0;
  for (counter = 0; counter < size_ - delim.size_ + 1; counter++) {
    if (memcmp(string_ + counter, delim.string_, delim.size_) == 0) {
      result.push_back(part);
      part.Clear();
      counter += delim.size_ - 1;
    } else {
      part.PushBack(string_[counter]);
    }
  }
  for (int i = counter; i < size_; i++) {
    part.PushBack(string_[i]);
  }
  result.push_back(part);
  return result;
}
String String::Join(const std::vector<String>& str) const {
  String res("");
  if (!str.empty()) {
    for (size_t i = 0; i < str.size(); i++) {
      res += str[i];
      res += *this;
    }
    res.size_ -= this->size_;
  }
  if (res.size_ != 0) {
    res.string_[res.size_] = '\0';
  }
  return res;
}
void String::Zero() { string_[size_] = '\0'; }
