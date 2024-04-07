#pragma once

#include "aligned_alloc.h"
#include "assertion.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <tuple>

namespace util
{

/**
 Structure of arrays implementation.

 Given a struct of elements, for example:

 struct MyStruct
 {
    float f;
    double d;
    std::string s;
 };

 The internal representation when converting to SOA format would be:

 struct MySOA
 {
    float* f_soa;
    double* d_soa;
    std::string* s_soa;
 };

 An example usage of this class using the above example:

 using Soa<float, double, std::string> Soa;
 Soa soa {1};
 soa.push_back(2.0f, 1.0, "One");
 auto elementOne = soa.data<0>();
 float f = elementOne[0];

 @tparam Elements The elements this soa instance will hold.
 */
template <typename... Elements>
class Soa
{
public:
    static constexpr size_t ElementCount = sizeof...(Elements);

    // type of the Nth array
    template <size_t N>
    using TypeN = typename std::tuple_element_t<N, std::tuple<Elements...>>;

    Soa() : mSize(0), mCapacity(0) {}

    explicit Soa(size_t size) : mSize(0), mCapacity(0) { reserve(size); }

    ~Soa()
    {
        destroy(0, mSize);
        align_free(std::get<0>(mArrays));
    }

    Soa(const Soa&) = delete;
    Soa& operator=(const Soa&) = delete;
    Soa(Soa&&) noexcept = default;
    Soa& operator=(Soa&&) noexcept = default;

    void push_back(const Elements&... elements) noexcept
    {
        pushBackImpl(elements..., std::make_index_sequence<sizeof...(Elements)>());
    }

    void push_back(Elements&&... elements) noexcept
    {
        pushBackImpl(
            std::forward<Elements>(elements)..., std::make_index_sequence<sizeof...(Elements)>());
    }

    void clear() noexcept
    {
        destroy(0, mSize);
        mSize = 0;
    }

    void reserve(size_t size) noexcept { checkCapacity(size); }

    void resize(size_t size) noexcept
    {
        checkCapacity(size);
        if (size < mSize)
        {
            destroy(size, mSize);
        }
        else if (size > mSize)
        {
            for_each([&size, this](auto* elementPtr) {
                using Type = typename std::decay<decltype(*elementPtr)>::type;
                if constexpr (!std::is_trivially_destructible_v<Type>)
                {
                    for (size_t idx = mSize; idx < size; ++idx)
                    {
                        new (elementPtr + idx) Type();
                    }
                }
            });
        }
        mSize = size;
    }

    template <size_t ElementIdx>
    TypeN<ElementIdx>* data() noexcept
    {
        static_assert(ElementIdx < ElementCount, "Element index out of range.");
        return std::get<ElementIdx>(mArrays);
    }

    template <size_t ElementIdx>
    TypeN<ElementIdx>* data() const noexcept
    {
        static_assert(ElementIdx < ElementCount, "Element index out of range.");
        return std::get<ElementIdx>(mArrays);
    }

    template <size_t ElementIdx>
    TypeN<ElementIdx>& at(size_t idx)
    {
        static_assert(ElementIdx < ElementCount, "Element index out of range.");
        ASSERT_FATAL(idx < mSize, "Out of range index value. (%i > %i)", idx, mSize);
        return std::get<ElementIdx>(mArrays)[idx];
    }

    template <size_t ElementIdx>
    TypeN<ElementIdx>& at(size_t idx) const
    {
        static_assert(ElementIdx < ElementCount, "Element index out of range.");
        ASSERT_FATAL(idx < mSize, "Out of range index value. (%i > %i)", idx, mSize);
        return std::get<ElementIdx>(mArrays)[idx];
    }

    template <size_t ElementIdx>
    TypeN<ElementIdx>* begin() noexcept
    {
        static_assert(ElementIdx < ElementCount, "Element index out of range.");
        return std::get<ElementIdx>(mArrays);
    }

    template <size_t ElementIdx>
    TypeN<ElementIdx>* end() noexcept
    {
        static_assert(ElementIdx < ElementCount, "Element index out of range.");
        return std::get<ElementIdx>(mArrays) + mSize;
    }

    [[nodiscard]] size_t size() const noexcept { return mSize; }
    [[nodiscard]] size_t capacity() const noexcept { return mCapacity; }
    [[nodiscard]] bool empty() const noexcept { return !mSize; }

private:
    void checkCapacity(size_t size) noexcept
    {
        if (size > mCapacity)
        {
            auto* oldBuffer = std::get<0>(mArrays);
            constexpr size_t elementSizeBytes = (sizeof(Elements) + ... + 0);
            const size_t newBufferSize = (elementSizeBytes * 2) * size;
            constexpr size_t maxAlign =
                std::max({std::max(alignof(std::max_align_t), alignof(Elements))...});
            auto* newBuffer = reinterpret_cast<uint8_t*>(aligned_alloc(maxAlign, newBufferSize));

            moveImpl(newBuffer, size);
            align_free(oldBuffer);

            mCapacity = size;
        }
    }

    template <typename Func, typename... Args>
    void for_each(Func&& func, Args&&... args)
    {
        forEachImpl(
            mArrays, [&](auto* elementPtr) { func(elementPtr, std::forward<Args>(args)...); });
    }

    // for_each implementation adapted from:
    // https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
    template <std::size_t I = 0, typename FuncT, typename... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    forEachImpl(std::tuple<Tp...>&, FuncT)
    {
    }

    template <std::size_t I = 0, typename FuncT, typename... Tp>
        inline typename std::enable_if <
        I<sizeof...(Tp), void>::type forEachImpl(std::tuple<Tp...>& t, FuncT f)
    {
        f(std::get<I>(t));
        forEachImpl<I + 1, FuncT, Tp...>(t, f);
    }

    void moveImpl(uint8_t* newBuffer, size_t size) noexcept
    {
        // compute aligned offset for each element.
        const size_t alignments[] {std::max(alignof(std::max_align_t), alignof(Elements))...};
        const size_t elementSizes[] {(sizeof(Elements) * size)...};

        size_t idx = 0;
        uint8_t* currentPtr = newBuffer;
        uint8_t* alignedPtrs[ElementCount];

        for_each(
            [&idx, &alignments, &elementSizes, &currentPtr, &alignedPtrs, this](auto* elementPtr) {
                using Type = typename std::decay<decltype(*elementPtr)>::type;
                Type* typedPtr = reinterpret_cast<Type*>(currentPtr);
                alignedPtrs[idx] = reinterpret_cast<uint8_t*>(
                    (std::uintptr_t(typedPtr) + (alignments[idx] - 1)) & ~(alignments[idx] - 1));
                currentPtr = reinterpret_cast<uint8_t*>(
                    std::uintptr_t(alignedPtrs[idx]) + std::uintptr_t(elementSizes[idx]));

                if (mSize)
                {
                    if constexpr (
                        !std::is_trivially_constructible_v<Type> ||
                        !std::is_trivially_destructible_v<Type>)
                    {
                        Type* alignPtr = reinterpret_cast<Type*>(alignedPtrs[idx]);
                        for (size_t offset = 0; offset < mSize; ++offset)
                        {
                            new (alignPtr + offset) Type(std::move(elementPtr[offset]));
                            if constexpr (!std::is_trivially_destructible_v<Type>)
                            {
                                elementPtr[offset].~Type();
                            }
                        }
                    }
                    else
                    {
                        memcpy(alignedPtrs[idx], elementPtr, mSize * sizeof(Type));
                    }
                }
                idx++;
            });

        idx = 0;
        forEachImpl(mArrays, [alignedPtrs, &idx](auto&& elementPtr) {
            using Type = typename std::remove_reference_t<decltype(elementPtr)>;
            elementPtr = reinterpret_cast<Type>(alignedPtrs[idx++]);
        });
    }

    template <size_t... Indices>
    void pushBackImpl(const Elements&... elements, std::index_sequence<Indices...>) noexcept
    {
        checkCapacity(mSize + 1);
        ([&] { new (std::get<Indices>(mArrays) + mSize) Elements {elements}; }(), ...);
        ++mSize;
    }

    template <size_t... Indices>
    void pushBackImpl(Elements&&... elements, std::index_sequence<Indices...>) noexcept
    {
        checkCapacity(mSize + 1);
        (
            [&] {
                new (std::get<Indices>(mArrays) + mSize)
                    Elements {std::forward<Elements>(elements)};
            }(),
            ...);
        ++mSize;
    }

    void destroy(size_t startIdx, size_t endIdx)
    {
        ASSERT_FATAL(endIdx <= mSize, "Out of range for end index.");
        for_each([&startIdx, &endIdx](auto* elementPtr) {
            using Type = typename std::decay<decltype(*elementPtr)>::type;
            if constexpr (!std::is_trivially_destructible_v<Type>)
            {
                for (size_t idx = startIdx; idx < endIdx; ++idx)
                {
                    elementPtr[idx++].~Type();
                }
            }
        });
    }

private:
    std::tuple<std::add_pointer_t<Elements>...> mArrays {};
    size_t mSize;
    size_t mCapacity;
};

} // namespace util