#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <iostream>
#include "array_ptr.h"

using namespace std;

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity) : capacity_(capacity) {
    }
    size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size) :
        items_(size),
        size_(size),
        capacity_(size) {
        std::generate(begin(), end(), []() {return Type(); });
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) :
        items_(size),
        size_(size),
        capacity_(size) {
        std::fill(begin(), end(), value);
    }

    SimpleVector(size_t size, Type&& value) :
        items_(size),
        size_(size),
        capacity_(size) {
        std::generate(begin(), end(), [value]() {return move(value); });
    }

    //Копипурет другой вектор
    SimpleVector(const SimpleVector& other) :
        items_(other.capacity_),
        size_(other.size_),
        capacity_(other.capacity_) {
        std::copy(other.begin(), other.end(), begin());
    }

    SimpleVector(SimpleVector&& other) {
        swap(other);
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) :
        items_(init.size()),
        size_(init.size()),
        capacity_(init.size()) {
        ArrayPtr<Type> temp(init.size());
        std::copy(init.begin(), init.end(), temp.Get());
        items_.swap(temp);
    }

    SimpleVector(ReserveProxyObj obj) {
        Reserve(obj.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        return items_[index];
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (*this != rhs) {
            Resize(rhs.size_);
            std::copy(rhs.begin(), rhs.end(), begin());
        }
        return *this;
    }

    SimpleVector& operator=(SimpleVector&& rhs) {
        if (*this != rhs) {
            swap(rhs);
        }
        return *this;
    }


    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        if (index >= size_)
            throw std::out_of_range("index must be in range 0, size"s);
        return items_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        if (index >= size_)
            throw std::out_of_range("index must be in range 0, size"s);
        return items_[index];
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        Resize(0);
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size) {
        if (new_size == size_) return;
        if (new_size < size_) {
            size_ = new_size;
            return;
        }
        if (new_size > size_ && new_size <= capacity_) {
            std::generate(begin() + size_, begin() + new_size, []() {return Type{}; });
            size_ = new_size;
            return;
        }
        if (new_size > capacity_) {
            new_size = std::max(new_size, capacity_ * 2);
            SimpleVector<Type> temp(new_size); // temp.size_ == new_size
            std::move(begin(), end(), temp.begin());
            swap(temp);
            return;
        }
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            SimpleVector<Type> temp(new_capacity); // temp.size_ == new_size
            std::move(begin(), end(), temp.begin());
            items_.swap(temp.items_);
            std::swap(capacity_, temp.capacity_);
        }
    }

    // Добавляет элемент в конец вектора
   // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        if (capacity_ == 0) {
            Reserve(1);
        }
        if (size_ < capacity_) {
            items_[size_] = item;
            ++size_;
            return;
        }
        if (size_ == capacity_) {
            Reserve(capacity_ * 2);
            PushBack(item);
            return;
        }
    }

    void PushBack(Type&& item) {
        if (capacity_ == 0) {
            Reserve(1);
        }
        if (size_ < capacity_) {

            items_[size_] = std::move(item);
            ++size_;
            return;
        }
        if (size_ == capacity_) {
            Reserve(capacity_ * 2);
            PushBack(move(item));
            return;
        }
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        size_t index = std::distance(cbegin(), pos);
        if (size_ == capacity_) {
            size_t temp_size = size_;
            Resize(capacity_ * 2);
            size_ = temp_size;
        }
        if (size_ == index) {
            PushBack(value);
            return begin() + index;
        }

        SimpleVector<Type> temp(capacity_);
        std::copy(begin(), begin() + index, temp.begin());
        temp[index] = value;
        std::copy(begin() + index, end(), temp.begin() + index + 1);
        items_.swap(temp.items_);
        ++size_;
        return begin() + index;
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        size_t index = std::distance(cbegin(), pos);
        if (size_ == capacity_) {
            size_t temp_size = size_;
            Resize(capacity_ * 2);
            size_ = temp_size;
        }
        if (size_ == index) {
            PushBack(move(value));
            return begin() + index;
        }

        SimpleVector<Type> temp(capacity_);
        std::move(begin(), begin() + index, temp.begin());
        temp[index] = move(value);
        std::move(begin() + index, end(), temp.begin() + index + 1);
        items_.swap(temp.items_);
        ++size_;
        return begin() + index;
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        if (size_ == 0) return;
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        size_t index = std::distance(cbegin(), pos);
        std::move(begin() + index + 1, end(), begin() + index);
        --size_;
        return begin() + index;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        items_.swap(other.items_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void swap(SimpleVector&& other) noexcept {
        std::move(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()), begin());
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return Iterator(items_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return Iterator(items_.Get() + size_);
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return ConstIterator(items_.Get());
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return ConstIterator(items_.Get() + size_);
    }

private:
    ArrayPtr<Type> items_;

    size_t size_ = 0;
    size_t capacity_ = 0;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (&lhs == &rhs) || (lhs.GetSize() == rhs.GetSize() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end()));
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs > rhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(rhs.begin(), rhs.end(), lhs.begin(), lhs.end());
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}
