#include "big_integer.hpp"

BigInt::BigInt(const std::string& copy) : is_negative_(copy[0] == '-') {
  size_t min_str;
  size_t size = copy.size();
  if (copy == "-0") {
    is_negative_ = false;
    big_int_.push_back(0);
  } else {
    size_t start = static_cast<size_t>(is_negative_);
    for (size_t iii = size; iii > start; iii -= kBase) {
      min_str = std::min(kBase, iii - start);
      big_int_.push_back(stoi(copy.substr(iii - min_str, min_str)));
      if (iii < kBase) {
        break;
      }
    }
  }
}

BigInt::BigInt(const int64_t& second) : BigInt(std::to_string(second)) {}

BigInt::BigInt(const BigInt& second)
    : is_negative_(second.is_negative_), big_int_(second.big_int_) {}

BigInt& BigInt::operator+=(const BigInt& second) {
  Arithmetic(second, true);
  return *this;
}

BigInt BigInt::operator+(const BigInt& second) const {
  BigInt copy = *this;
  copy += second;
  return copy;
}

BigInt& BigInt::operator-=(const BigInt& second) {
  Arithmetic(second, false);
  return *this;
}

BigInt BigInt::operator-(const BigInt& second) const {
  BigInt copy = *this;
  copy -= second;
  return copy;
}

BigInt& BigInt::operator*=(const BigInt& second) {
  BigInt time_my_big_int;
  BigInt mult_rez;
  bool copy_not_negative = (is_negative_ != second.is_negative_);
  for (size_t count = 0; count < big_int_.size(); ++count) {
    for (size_t mult = 0; mult < second.big_int_.size(); ++mult) {
      time_my_big_int = big_int_[count] * second.big_int_[mult];
      mult_rez += Mult(time_my_big_int, count + mult);
    }
  }
  *this = mult_rez;
  if (*this != 0) {
    is_negative_ = copy_not_negative;
  } else {
    is_negative_ = false;
  }
  return *this;
}

BigInt BigInt::operator*(const BigInt& second) const {
  BigInt copy = *this;
  copy *= second;
  return copy;
}

BigInt BigInt::operator/(const BigInt& second) const {
  BigInt copy = *this;
  copy /= second;
  return copy;
}

BigInt& BigInt::operator/=(const BigInt& second) {
  BigInt copy_this = *this;
  copy_this.is_negative_ = false;
  BigInt copy_second = second;
  copy_second.is_negative_ = false;
  BigInt left = 1;
  BigInt right = copy_this + 1;
  BinSearch(copy_this, copy_second, left, right);
  right -= 1;
  if (right != 0) {
    right.is_negative_ = (is_negative_ != second.is_negative_);
  }
  *this = right;
  return *this;
}

bool BigInt::operator==(const BigInt& second) const {
  if (is_negative_ != second.is_negative_) {
    return false;
  }
  return !(*this < second) && !(second < *this);
}

bool BigInt::operator<(const BigInt& second) const {
  if (second.big_int_.empty() && big_int_.empty()) {
    return false;
  }
  if (is_negative_ != second.is_negative_) {
    return (is_negative_);
  }
  if (second.big_int_.size() != big_int_.size()) {
    return (second.big_int_.size() > big_int_.size() != (is_negative_));
  }
  for (int iii = big_int_.size() - 1; iii >= 0; --iii) {
    if (big_int_[iii] != second.big_int_[iii]) {
      if (is_negative_) {
        return big_int_[iii] > second.big_int_[iii];
      }
      return big_int_[iii] < second.big_int_[iii];
    }
  }
  return false;
}
bool BigInt::operator>(const BigInt& second) const { return (second < *this); }

bool BigInt::operator!=(const BigInt& second) const {
  return !(*this == second);
}

bool BigInt::operator<=(const BigInt& second) const {
  return (*this < second || *this == second);
}

bool BigInt::operator>=(const BigInt& second) const {
  return (*this > second || *this == second);
}

BigInt BigInt::operator%=(const BigInt& second) {
  *this -= (*this / second) * second;
  return *this;
}

BigInt BigInt::operator%(const BigInt& second) const {
  BigInt copy = *this;
  copy %= second;
  return copy;
}

BigInt BigInt::operator=(const BigInt& second) {
  big_int_ = second.big_int_;
  is_negative_ = second.is_negative_;
  return *this;
}
BigInt BigInt::operator--(int) {
  BigInt num;
  num = *this;
  --(*this);
  return num;
}

BigInt BigInt::operator++(int) {
  BigInt num;
  num = *this;
  ++(*this);
  return num;
}

BigInt& BigInt::operator--() {
  *this -= 1;
  return *this;
}

BigInt& BigInt::operator++() {
  *this += 1;
  return *this;
}

BigInt BigInt::operator-() {
  if (big_int_.size() == 1 && big_int_.back() == 0) {
    return *this;
  }
  BigInt copy = *this;
  copy.is_negative_ = !(copy.is_negative_);
  return copy;
}

BigInt BigInt::DivByTwo(const BigInt& my_big_int) {
  BigInt copy = my_big_int.big_int_[0] / 2;
  BigInt time_r = my_big_int;
  time_r.big_int_.erase(time_r.big_int_.begin());
  copy += time_r * __fordiv__;
  return copy;
}

std::ostream& operator<<(std::ostream& oos, const BigInt& output) {
  const int kBase = 9;
  if (output.big_int_.empty()) {
    return oos;
  }
  if (output.is_negative_ &&
      static_cast<bool>(output.big_int_[output.big_int_.size() - 1])) {
    oos << "-";
  }
  oos << output.big_int_[output.big_int_.size() - 1];
  for (int64_t iii = output.big_int_.size() - 2; iii >= 0; iii--) {
    std::string zer = std::to_string(output.big_int_[iii]);
    for (size_t ppp = 0; ppp < kBase - zer.length(); ++ppp) {
      oos << '0';
    }
    oos << output.big_int_[iii];
  }
  return oos;
}

std::istream& operator>>(std::istream& iin, BigInt& input) {
  std::string second_str;
  iin >> second_str;
  input = BigInt(second_str);
  if ((input.big_int_.size() == 1) && (input.big_int_[0] == 0)) {
    input.is_negative_ = false;
  }
  return iin;
}
void BigInt::CopyPastPlus1(int count, int& over, size_t size) {
  big_int_[size] += count;
  over = big_int_[size] / kDegOfBase;
  big_int_[size] %= kDegOfBase;
}
void BigInt::CopyPastPlus2(int& over, int count, size_t size) {
  big_int_.push_back(count);
  over = big_int_[size] / kDegOfBase;
  big_int_[size] %= kDegOfBase;
}

void BigInt::Plus(BigInt& first, BigInt second, bool not_negative) {
  int over = 0;
  size_t iii = 0;
  size_t min_len = std::min(first.big_int_.size(), second.big_int_.size());
  for (; iii < min_len; ++iii) {
    first.CopyPastPlus1(second.big_int_[iii] + over, over, iii);
  }
  for (; iii < first.big_int_.size(); ++iii) {
    first.CopyPastPlus1(over, over, iii);
  }
  for (; iii < second.big_int_.size(); ++iii) {
    first.CopyPastPlus2(over, second.big_int_[iii] + over, iii);
  }
  if (over != 0) {
    first.big_int_.push_back(over);
  }
  this->is_negative_ = not_negative;
}

void BigInt::CopyPastMinus(int checker, int& over, size_t count) {
  big_int_[count] -= checker;
  Change(big_int_[count], over);
}

BigInt BigInt::Minus(BigInt& first, BigInt second, bool not_negative) {
  int over = 0;
  size_t count = 0;
  this->is_negative_ = not_negative;
  for (; count < second.big_int_.size(); ++count) {
    first.CopyPastMinus(second.big_int_[count] + over, over, count);
  }
  for (; count < first.big_int_.size(); ++count) {
    first.CopyPastMinus(over, over, count);
  }
  while ((first.big_int_[count - 1] == 0) && (count != 1)) {
    first.big_int_.pop_back();
    --count;
  }
  return first;
}

void BigInt::Change(long long& big_int_count, int& over) const {
  if (big_int_count < 0) {
    big_int_count += kDegOfBase;
    over = 1;
  } else {
    over = 0;
  }
}

BigInt BigInt::Mult(BigInt& my_big_int, int rank) {
  if (my_big_int.big_int_[my_big_int.big_int_.size() - 1] != 0) {
    for (int iii = 0; iii < rank; ++iii) {
      my_big_int.big_int_.insert(my_big_int.big_int_.begin(), 0);
    }
  }
  return my_big_int;
}
void BigInt::Arithmetic(const BigInt& second, bool oper_plus) {
  if (oper_plus) {
    if (is_negative_ == second.is_negative_) {
      Plus(*this, second, is_negative_);
    } else if (((*this > second) == is_negative_) || (*this == second)) {
      Minus(*this, second, is_negative_);
    } else {
      *this = (second - (-*this));
    }
  } else {
    if (is_negative_ != second.is_negative_) {
      Plus(*this, second, is_negative_);
    } else if (((*this < second) == is_negative_) || (*this == second)) {
      Minus(*this, second, is_negative_);
    } else {
      bool tmp_sign = !(is_negative_);
      *this = (second - *this);
      is_negative_ = tmp_sign;
    }
  }
}
BigInt BigInt::BinSearch(BigInt k_first, BigInt k_second, BigInt left,
                         BigInt& right) {
  BigInt minus;
  while (left < right) {
    BigInt mid = (DivByTwo(--(right + left)));
    minus = k_first - (k_second * mid);
    if (minus < 0) {
      right = mid;
    } else {
      left = mid + 1;
    }
  }
  return right;
}