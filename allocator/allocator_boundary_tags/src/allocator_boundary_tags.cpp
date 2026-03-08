#include <not_implemented.h>
#include "../include/allocator_boundary_tags.h"

// | Global metadata |   free    |   occupied    |

// Sizes of global metadata
constexpr size_t aligner(size_t value, size_t alignment) noexcept
{
    return (value + alignment - 1) / alignment * alignment;
}

// | p_a* | size | mutex | f_m | f_b* |
constexpr size_t parent_allocator_offset = 0;
constexpr size_t size_offset = aligner(parent_allocator_offset + sizeof(std::pmr::memory_resource*), alignof(size_t));
constexpr size_t mutex_offset = aligner(size_offset + sizeof(size_t), alignof(std::mutex));
constexpr size_t fit_mode_offset = aligner(mutex_offset + sizeof(std::mutex), alignof(allocator_with_fit_mode::fit_mode));
constexpr size_t first_oc_block_offset = aligner(fit_mode_offset + sizeof(allocator_with_fit_mode::fit_mode), alignof(void*));

constexpr size_t global_meta_size = sizeof(std::pmr::memory_resource*) + sizeof(size_t) + sizeof(std::mutex) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(void*);

// Sizes of occupied blocks' metadata
// | backward* | forward* | size | p* |
constexpr size_t backward_oc_block_offset = 0
constexpr size_t forward_oc_block_offset = aligner(backward_oc_block_offset + sizeof(void*), alignof(void*));
constexpr size_t size_of_block_offset = aligner(forward_oc_block_offset + sizeof(void*), alignof(size_t));
constexpr size_t parent_of_block_offset = aligner(size_of_block_offset + sizeof(size_t), alignof(void*));

constexpr size_t = oc_block_meta_size = sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*);

std::byte *ptr_to_bytes(void *ptr) noexcept
{
    return reinterpret_cast<std::byte*>(ptr);
}

const std::byte *ptr_to_bytes(const void *ptr) noexcept
{
    return reinterpret_cast<const std::byte*>(ptr);
}

template<typename T>
T &access_atribute(void *start, size_t offset)
{
    return *reinterpret_cast<T*>(ptr_to_bytes(start) + offset);
}

template<typename T>
const T &access_atribute(const void *start, size_t offset)
{
    return *reinterpret_cast<const T*>(ptr_to_bytes(start) + offset);
}

void *allocate_memory_block(size_t space_size, std::pmr::memory_resource *parent_allocator) noexcept
{
    if (parent_allocator == nullptr)
    {
        return parent_allocator->allocate(space_size, std::max_align_t);
    }

    return ::operator new(space_size);
}

void release_memory_block(void *start) noexcept
{
    if (start == nullptr)
    {
        return;
    }

    auto *parent_allocator = access_atribute<std::pmr::memory_resource*>(start, parent_allocator_offset);
    size_t size = access_atribute<size_t>(start, size_offset);
    access_atribute<std::mutex>(start, mutex_offset).~mutex();

    if (parent_allocator == nullptr)
    {
        ::operator delete(start);
    } 
    else
    {
        parent_allocator->deallocate(start, size, std::max_align_t);
    }
}

allocator_boundary_tags::~allocator_boundary_tags()
{
    if (_trusted_memory == nullptr) return;

    _trusted_memory = nullptr;
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept : _trusted_memory(other._trusted_memory)
{
    other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    throw not_implemented("allocator_boundary_tags &allocator_boundary_tags::operator=(allocator_boundary_tags &&) noexcept", "your code should be here...");
}


/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    throw not_implemented("allocator_boundary_tags::allocator_boundary_tags(size_t,std::pmr::memory_resource *,logger *,allocator_with_fit_mode::fit_mode)", "your code should be here...");
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t size)
{
    throw not_implemented("[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(size_t)", "your code should be here...");
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
    throw not_implemented("void allocator_boundary_tags::do_deallocate_sm(void *)", "your code should be here...");
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    throw not_implemented("inline void allocator_boundary_tags::set_fit_mode(allocator_with_fit_mode::fit_mode)", "your code should be here...");
}


std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const", "your code should be here...");
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept
{
    throw not_implemented("allocator_boundary_tags::boundary_iterator allocator_boundary_tags::begin() const noexcept", "your code should be here...");
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept
{
    throw not_implemented("allocator_boundary_tags::boundary_iterator allocator_boundary_tags::end() const noexcept", "your code should be here...");
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info_inner() const", "your code should be here...");
}

allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)
{
    throw not_implemented("allocator_boundary_tags::allocator_boundary_tags(const allocator_boundary_tags &other)", "your code should be here...");
}

allocator_boundary_tags &allocator_boundary_tags::operator=(const allocator_boundary_tags &other)
{
    if (this == &other) return *this;

    allocator_boundary_tags temp(other);
    *this = std::move(temp);

    return *this;
}

bool allocator_boundary_tags::do_is_equal(const std::pmr::memory_resource &other) const noexcept
{
    return this == &other;
}

bool allocator_boundary_tags::boundary_iterator::operator==(
        const allocator_boundary_tags::boundary_iterator &other) const noexcept
{
    return _occupied_ptr == other._occupied_ptr && _occupied == other._occupied && _trusted_memory == other._trusted_memory;
}

bool allocator_boundary_tags::boundary_iterator::operator!=(
        const allocator_boundary_tags::boundary_iterator & other) const noexcept
{
    return !(*this == other);
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept
{
    throw not_implemented("allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator++() & noexcept", "your code should be here...");
}

allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept
{
    throw not_implemented("allocator_boundary_tags::boundary_iterator &allocator_boundary_tags::boundary_iterator::operator--() & noexcept", "your code should be here...");
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator++(int n)
{
    auto copy = *this;
    ++(*this);
    return copy;
}

allocator_boundary_tags::boundary_iterator allocator_boundary_tags::boundary_iterator::operator--(int n)
{
    auto copy = *this;
    --(*this);
    return copy;
}

size_t allocator_boundary_tags::boundary_iterator::size() const noexcept
{
    throw not_implemented("size_t allocator_boundary_tags::boundary_iterator::size() const noexcept", "your code should be here...");
}

bool allocator_boundary_tags::boundary_iterator::occupied() const noexcept
{
    return _occupied;
}

void* allocator_boundary_tags::boundary_iterator::operator*() const noexcept
{
    return get_ptr();
}

allocator_boundary_tags::boundary_iterator::boundary_iterator() : _occupied_ptr(nullptr), _occupied(false), _trusted_memory(nullptr)
{
}

allocator_boundary_tags::boundary_iterator::boundary_iterator(void *trusted)
{
    throw not_implemented("allocator_boundary_tags::boundary_iterator::boundary_iterator(void *)", "your code should be here...");
}

void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept
{
    throw not_implemented("void *allocator_boundary_tags::boundary_iterator::get_ptr() const noexcept", "your code should be here...");
}
