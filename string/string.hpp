#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class String {
 private:
  int size_ = 0;
  int capacity_ = 0;
  char* string_ = nullptr;

 public:
  String() = default;
  String(int size, char letter);
  String(const char* cstr);
  String(const String& other);
  ~String() { delete[] string_; };
  size_t Size() const;
  size_t Capacity() const;
  void Clear();
  bool Empty() const;
  void PushBack(const char& character);
  void PopBack();
  void Resize(int new_size);
  void Resize(int new_size, char character);
  void Reserve(int new_cap);
  void ShrinkToFit();
  void Swap(String& other);
  char& operator[](size_t iii);
  const char& operator[](size_t iii) const;
  char& Front();
  char& Back();
  char Front() const;
  char Back() const;
  const char* Data() const;
  char* Data();
  bool operator<(String other) const;
  bool operator>(String other) const;
  bool operator<=(String other) const;
  bool operator>=(String other) const;
  bool operator==(const String& other) const;
  bool operator!=(String other) const;
  char& operator[](int index);
  const char& operator[](int index) const;
  String& operator+=(const String& other);
  friend String operator+(const String& first_string,
                          const String& second_string);
  String& operator=(const String& other);
  friend String operator*(const String& string, int n);
  String& operator*=(int n);
  friend std::istream& operator>>(std::istream& iis, String& other);
  friend std::ostream& operator<<(std::ostream& oos, const String& other);
  std::vector<String> Split(const String& delim = " ");
  String Join(const std::vector<String>& str) const;
  void Zero();
};
