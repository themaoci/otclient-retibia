#include <memory>
#include <stdexcept>

struct anyType
{
    anyType() = default;
    template <typename T> anyType(T const& v) : _storage(new storage<T>(v)) { }
    anyType(anyType const& other) : _storage(other._storage ? std::move(other._storage->clone()) : nullptr) {}

    void swap(anyType& other) { _storage.swap(other._storage); }
    friend void swap(anyType& a, anyType& b) { a.swap(b); };
    anyType& operator=(anyType other) { swap(other); return *this; }

    // todo move semantics
private:
    struct storage_base {
        virtual std::unique_ptr<storage_base> clone() = 0;
        virtual ~storage_base() = default;
    };
    template <typename T>
    struct storage : storage_base {
        T value;
        explicit storage(T const& v) : value(v) {}
        std::unique_ptr<storage_base> clone() { return std::unique_ptr<storage_base>(new storage<T>(value)); }
    };
    std::unique_ptr<storage_base> _storage;
    template<typename T> friend T& any_cast(anyType&);
    template<typename T> friend T const& any_cast(anyType const&);
};

template <typename T> T& any_cast(anyType& a) {
    if (auto p = dynamic_cast<anyType::storage<T>*>(a._storage.get()))
        return p->value;
    else
        throw std::bad_cast();
}

template <typename T> T const& any_cast(anyType const& a) {
    if (auto p = dynamic_cast<anyType::storage<T> const*>(a._storage.get()))
        return p->value;
    else
        throw std::bad_cast();
}