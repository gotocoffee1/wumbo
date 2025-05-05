#pragma once

#include <memory>

namespace wumbo
{
template<typename T, typename Deleter = std::default_delete<T>>
class box
{
    std::unique_ptr<T, Deleter> _impl;

  public:
    using element_type = T;

  public:
    template<typename... Args>
    box(Args&&... args)
        : _impl(std::make_unique<T>(std::forward<Args>(args)...))
    {
    }

    box(const box& other)
        : box(*other._impl)
    {
    }

    box& operator=(const box& other)
    {
        if (this != &other)
            *_impl = *other._impl;
        return *this;
    }

    box(box&& other)
        : _impl(std::move(other._impl))
    {
    }

    box& operator=(box&& other)
    {
        _impl = std::move(other._impl);
        return *this;
    }

    ~box() = default;

    T& operator*()
    {
        return *_impl;
    }
    const T& operator*() const
    {
        return *_impl;
    }

    T* operator->()
    {
        return _impl.get();
    }
    const T* operator->() const
    {
        return _impl.get();
    }
};

template<typename T, typename Deleter = std::default_delete<T>>
class optional_box
{
    std::unique_ptr<T, Deleter> _impl;

  public:
    using element_type = T;

  public:
    optional_box()
    {
    }

    template<class U, class E>
    optional_box(std::unique_ptr<U, E>&& ptr)
        : _impl{std::move(ptr)}
    {
    }

    optional_box(optional_box&& other)            = default;
    optional_box& operator=(optional_box&& other) = default;

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(_impl);
    }

    optional_box(const optional_box& other)
        : optional_box(other ? std::make_unique<T>(*other._impl) : nullptr)
    {
    }

    optional_box& operator=(const optional_box& other)
    {
        if (this != &other)
        {
            if (_impl)
            {
                if (other)
                    *_impl = *other._impl;
                else
                    _impl = nullptr;
            }
            else if (other)
            {
                _impl = std::make_unique<T>(*other._impl);
            }
        }
        return *this;
    }

    ~optional_box() = default;

    T& operator*()
    {
        return *_impl;
    }
    const T& operator*() const
    {
        return *_impl;
    }

    T* operator->()
    {
        return _impl.get();
    }
    const T* operator->() const
    {
        return _impl.get();
    }
};

} // namespace tara
