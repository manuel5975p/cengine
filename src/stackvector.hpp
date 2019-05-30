#ifndef STACKVECTOR_HPP
#define STACKVECTOR_HPP
#include <array>
#include <cstddef>
#include <cassert>
template <typename T, std::size_t _max_size> struct stackvector {
  typedef T value_type;
  typedef value_type *pointer;
  typedef const value_type *const_pointer;
  typedef value_type &reference;
  typedef const value_type &const_reference;
  typedef value_type *iterator;
  typedef const value_type *const_iterator;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  stackvector() : m_size(0) {}
  void fill(const value_type &u) { std::fill_n(begin(), size(), u); }
  void push_back(const value_type &v) { m_data[m_size++] = v; }
  void push_back(value_type &&v) { m_data[m_size++] = std::move(v); }
  void pop_back() { m_data[--m_size].~T(); }
  void swap(stackvector &other) {
    std::swap_ranges(begin(), end(), other.begin());
  }
  template<typename... Ts>
  void emplace_back(Ts... args){
    push_back(value_type(std::forward<Ts>(args)...));
  }
  constexpr iterator begin() noexcept { return iterator(data()); }

  constexpr const_iterator begin() const noexcept {
    return const_iterator(data());
  }

  constexpr iterator end() noexcept { return iterator(data() + m_size); }

  constexpr const_iterator end() const noexcept {
    return const_iterator(data() + m_size);
  }

  constexpr reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }

  constexpr const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  constexpr reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }

  constexpr const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  constexpr const_iterator cbegin() const noexcept {
    return const_iterator(data());
  }

  constexpr const_iterator cend() const noexcept {
    return const_iterator(data() + m_size);
  }

  constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  constexpr size_type size() const noexcept { return m_size; }

  constexpr size_type max_size() const noexcept { return m_max_size; }

  constexpr const T* data() const noexcept { return m_data; }

  constexpr T* data() noexcept { return m_data; }

  constexpr bool empty() const noexcept { return size() == 0; }
  
  T& operator[](size_type i) noexcept{assert(i < size());return m_data[i];}
  const T& operator[](size_type i)const noexcept{assert(i < size());return m_data[i];}
private:
  std::size_t m_size;
  constexpr static std::size_t m_max_size = _max_size;
  T m_data[m_max_size];
};
#endif
