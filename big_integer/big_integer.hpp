#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#define __fordiv__ 500000000ll

class BigInt {
 public:
  BigInt() = default;
  explicit BigInt(const std::string& copy);
  BigInt(const int64_t& second);
  BigInt(const BigInt& second);
  BigInt operator=(const BigInt& second);
  BigInt operator-();
  bool operator==(const BigInt& second) const;
  bool operator<(const BigInt& second) const;
  bool operator>(const BigInt& second) const;
  bool operator!=(const BigInt& second) const;
  bool operator<=(const BigInt& second) const;
  bool operator>=(const BigInt& second) const;
  BigInt& operator+=(const BigInt& second);
  BigInt operator+(const BigInt& second) const;
  BigInt& operator-=(const BigInt& second);
  BigInt operator-(const BigInt& second) const;
  BigInt& operator*=(const BigInt& second);
  BigInt operator*(const BigInt& second) const;
  BigInt& operator/=(const BigInt& second);
  BigInt operator/(const BigInt& second) const;
  BigInt operator%=(const BigInt& second);
  BigInt operator%(const BigInt& second) const;
  BigInt operator--(int);
  BigInt operator++(int);
  BigInt& operator++();
  BigInt& operator--();
  friend std::ostream& operator<<(std::ostream& oos, const BigInt& output);
  friend std::istream& operator>>(std::istream& iin, BigInt& input);

 private:
  std::vector<long long> big_int_;
  const size_t kBase = 9;
  const int kDegOfBase = 1e9;
  bool is_negative_ = false;
  BigInt static DivByTwo(const BigInt& my_big_int);
  void CopyPastPlus1(int count, int& over, size_t size);
  void CopyPastPlus2(int& over, int count, size_t size);
  void Plus(BigInt& first, BigInt second, bool not_negative);
  void CopyPastMinus(int checker, int& over, size_t count);
  BigInt Minus(BigInt& first, BigInt second, bool not_negative);
  void Change(long long& big_int_count, int& over) const;
  BigInt static Mult(BigInt& my_big_int, int rank);
  void Arithmetic(const BigInt& second, bool oper_plus);
  BigInt static BinSearch(BigInt k_first, BigInt k_second, BigInt left,
                          BigInt& right);
};