#pragma once

#include <iostream>
#include <vector>

template <size_t N, size_t M, typename T = int64_t>
class Matrix {
 private:
  std::vector<std::vector<T>> Data_;
  size_t rows_;
  size_t columns_;

 public:
  Matrix()
      : Data_(std::vector<std::vector<T>>(N, std::vector<T>(M))),
        rows_(N),
        columns_(M) {}

  Matrix(std::vector<std::vector<T>> mat)
      : Data_(mat), rows_(mat.size()), columns_(mat[0].size()) {}

  Matrix(T elem) : Matrix() {
    for (size_t i = 0; i < Data_.size(); i++) {
      for (size_t j = 0; j < Data_[0].size(); j++) {
        operator()(i, j) = elem;
      }
    }
  }

  Matrix(const Matrix& other)
      : rows_(other.rows_), columns_(other.columns_), Data_(other.Data_) {}

  Matrix& operator=(const Matrix& other) {
    if (Data_ != other.Data_) {
      rows_ = other.rows_;
      columns_ = other.columns_;
      Data_ = other.Data_;
    }
    return *this;
  }

  Matrix& operator+=(const Matrix& other) {
    for (size_t i = 0; i < rows_; i++) {
      for (size_t j = 0; j < columns_; j++) {
        operator()(i, j) += other.operator()(i, j);
      }
    }
    return *this;
  }

  Matrix operator+(const Matrix& other) {
    auto tmp = *this;
    tmp += other;
    return tmp;
  }

  Matrix& operator-=(const Matrix& other) {
    for (size_t i = 0; i < rows_; i++) {
      for (size_t j = 0; j < columns_; j++) {
        operator()(i, j) -= other.operator()(i, j);
      }
    }
    return *this;
  }

  Matrix operator-(const Matrix& other) {
    auto tmp = *this;
    tmp -= other;
    return tmp;
  }
  Matrix& operator*=(T elem) {
    for (size_t iii = 0; iii < rows_; iii++) {
      for (size_t jjj = 0; jjj < columns_; jjj++) {
        operator()(iii, jjj) *= elem;
      }
    }
    return *this;
  }

  Matrix operator==(const Matrix& other) { return (Data_ == other.Data_); }

  Matrix<M, N, T> Transposed() {
    Matrix<M, N, T> copy;
    for (size_t iii = 0; iii < columns_; ++iii) {
      for (size_t jjj = 0; jjj < rows_; ++jjj) {
        copy(iii, jjj) = operator()(jjj, iii);
      }
    }
    return copy;
  }

  T& operator()(size_t rows, size_t columns) { return Data_[rows][columns]; }

  T operator()(size_t rows, size_t columns) const {
    return Data_[rows][columns];
  }
};

template <size_t N, typename T>
class Matrix<N, N, T> {
 private:
  std::vector<std::vector<T>> Data_;
  size_t rows_;

 public:
  Matrix()
      : Data_(std::vector<std::vector<T>>(N, std::vector<T>(N))), rows_(N) {}

  Matrix(std::vector<std::vector<T>> mat) : Data_(mat), rows_(mat.size()) {}

  Matrix(T elem) : rows_(N) {
    Data_ = std::vector<std::vector<T>>(N);
    for (size_t i = 0; i < Data_.size(); i++) {
      for (size_t j = 0; j < Data_[0].size(); j++) {
        operator()(i, j) = elem;
      }
    }
  }

  Matrix(const Matrix& other) : rows_(other.rows_), Data_(other.Data_) {}

  Matrix& operator=(const Matrix& other) {
    rows_ = other.rows_;
    Data_ = other.Data_;
    return *this;
  }

  Matrix& operator+=(const Matrix& other) {
    for (size_t i = 0; i < rows_; i++) {
      for (size_t j = 0; j < rows_; j++) {
        operator()(i, j) += other.operator()(i, j);
      }
    }
    return *this;
  }

  Matrix operator+(const Matrix& other) {
    auto tmp = *this;
    tmp += other;
    return tmp;
  }

  Matrix& operator-=(const Matrix& other) {
    for (size_t iii = 0; iii < rows_; iii++) {
      for (size_t jjj = 0; jjj < rows_; jjj++) {
        operator()(iii, jjj) -= other.operator()(iii, jjj);
      }
    }
    return *this;
  }

  Matrix operator-(const Matrix& other) {
    auto tmp = *this;
    tmp -= other;
    return tmp;
  }
  Matrix& operator*=(T elem) {
    for (size_t iii = 0; iii < rows_; iii++) {
      for (size_t jjj = 0; jjj < rows_; jjj++) {
        operator()(iii, jjj) *= elem;
      }
    }
    return *this;
  }

  bool operator==(const Matrix& other) { return (Data_ == other.Data_); }

  Matrix<N, N, T> Transposed() {
    Matrix<N, N, T> copy;
    for (size_t iii = 0; iii < rows_; ++iii) {
      for (size_t jjj = 0; jjj < rows_; ++jjj) {
        copy(iii, jjj) = operator()(jjj, iii);
      }
    }
    return copy;
  }

  T Trace() {
    T res;
    for (size_t iii = 0; iii < rows_; ++iii) {
      res += operator()(iii, iii);
    }
    return res;
  }

  T& operator()(size_t rows, size_t columns) { return Data_[rows][columns]; }

  T operator()(size_t rows, size_t columns) const {
    return Data_[rows][columns];
  }
};

template <size_t N, size_t M, size_t K, typename T>
Matrix<N, K, T> operator*(const Matrix<N, M, T>& first,
                          const Matrix<M, K, T>& second) {
  Matrix<N, K, T> copy;
  for (size_t rows1 = 0; rows1 < N; ++rows1) {
    for (size_t columns1 = 0; columns1 < K; ++columns1) {
      for (size_t columns2 = 0; columns2 < M; ++columns2) {
        copy(rows1, columns1) +=
            first(rows1, columns2) * second(columns2, columns1);
      }
    }
  }
  return copy;
}
template <size_t N, size_t M, typename T>
Matrix<N, M, T> operator*(const Matrix<N, M, T>& first, const T& elem) {
  Matrix<N, M, T> copy = first;
  copy *= elem;
  return copy;
}