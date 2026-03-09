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
constexpr size_t backward_oc_block_offset = 0;
constexpr size_t forward_oc_block_offset = aligner(backward_oc_block_offset + sizeof(void*), alignof(void*));
constexpr size_t size_of_block_offset = aligner(forward_oc_block_offset + sizeof(void*), alignof(size_t));
constexpr size_t parent_of_block_offset = aligner(size_of_block_offset + sizeof(size_t), alignof(void*));

constexpr size_t oc_block_meta_size = sizeof(void*) + sizeof(void*) + sizeof(size_t) + sizeof(void*);

std::byte *ptr_to_bytes(void *ptr) noexcept
{
    return reinterpret_cast<std::byte*>(ptr);
}

const std::byte *ptr_to_bytes(const void *ptr) noexcept
{
    return reinterpret_cast<const std::byte*>(ptr);
}

template<typename T>
T &access_field(void *start, size_t offset)
{
    return *reinterpret_cast<T*>(ptr_to_bytes(start) + offset);
}

template<typename T>
const T &access_field(const void *start, size_t offset)
{
    return *reinterpret_cast<const T*>(ptr_to_bytes(start) + offset);
}

void *allocate_memory_block(size_t space_size, std::pmr::memory_resource *parent_allocator) noexcept
{
    if (parent_allocator == nullptr)
    {
        return parent_allocator->allocate(space_size, alignof(std::max_align_t));
    }

    return ::operator new(space_size);
}

void release_memory_block(void *start) noexcept
{
    if (start == nullptr)
    {
        return;
    }

    auto *parent_allocator = access_field<std::pmr::memory_resource*>(start, parent_allocator_offset);
    size_t size = access_field<size_t>(start, size_offset);
    access_field<std::mutex>(start, mutex_offset).~mutex();

    if (parent_allocator == nullptr)
    {
        ::operator delete(start);
    } 
    else
    {
        parent_allocator->deallocate(start, size, alignof(std::max_align_t));
    }
}

void init_memory_block(void *start, size_t space_size, std::pmr::memory_resource *parent_allocator, allocator_with_fit_mode::fit_mode fit_mode)
{
    access_field<std::pmr::memory_resource*>(start, parent_allocator_offset) = parent_allocator;
    access_field<size_t>(start, size_offset) = space_size;
    access_field<allocator_with_fit_mode::fit_mode>(start, fit_mode_offset) = fit_mode;

    new (&access_field<std::mutex>(start, mutex_offset)) std::mutex();

    access_field<void*>(start, first_oc_block_offset) = nullptr;
}

void *access_last_oc_block(void *start) noexcept
{
    void *curr = access_field<void*>(start, first_oc_block_offset);

    if (curr == nullptr) return nullptr;

    while(access_field<void*>(curr, forward_oc_block_offset) != nullptr)
    {
        curr = access_field<void*>(curr, forward_oc_block_offset);
    }

    return curr;
}

allocator_boundary_tags::~allocator_boundary_tags()
{
    release_memory_block(_trusted_memory);

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
    if (this != &other)
    {
        release_memory_block(_trusted_memory);
        _trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }

    return *this;
}

/** If parent_allocator* == nullptr you should use std::pmr::get_default_resource()
 */
allocator_boundary_tags::allocator_boundary_tags(
        size_t space_size,
        std::pmr::memory_resource *parent_allocator,
        allocator_with_fit_mode::fit_mode allocate_fit_mode) : _trusted_memory(nullptr)
{
    if (parent_allocator == nullptr)
    {
        parent_allocator == std::pmr::get_default_resource();
    }

    if (space_size < global_meta_size + oc_block_meta_size)
    {
        throw std::bad_alloc();
    }

    _trusted_memory = allocate_memory_block(space_size, parent_allocator);

    try 
    {
        init_memory_block(_trusted_memory, space_size, parent_allocator, allocate_fit_mode);
    }
    catch(...)
    {
        parent_allocator->deallocate(_trusted_memory, space_size);
        _trusted_memory = nullptr;
        throw;
    }
}

[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(
    size_t size)
{
    throw not_implemented("[[nodiscard]] void *allocator_boundary_tags::do_allocate_sm(size_t)", "your code should be here...");
}

void allocator_boundary_tags::do_deallocate_sm(
    void *at)
{
    if (at == nullptr) return;

    std::lock_guard<std::mutex> lock(access_field<std::mutex>(_trusted_memory, mutex_offset));

    void *block = ptr_to_bytes(at) - oc_block_meta_size;

    if (access_field<void*>(block, parent_of_block_offset) != _trusted_memory)
    {
        throw std::invalid_argument("Block doesn't belong to this allocator!");
    }

    void *prev = access_field<void*>(block, backward_oc_block_offset);
    void *next = access_field<void*>(block, forward_oc_block_offset);

    if (prev == nullptr) 
    {
        access_field<void*>(_trusted_memory, first_oc_block_offset) = next;
    }
    else
    {
        access_field<void*>(prev, forward_oc_block_offset) = next;
    }

    if (next != nullptr)
    {
        access_field<void*>(next, backward_oc_block_offset) = prev;
    }
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    std::lock_guard<std::mutex> lock(access_field<std::mutex>(_trusted_memory, mutex_offset));
    access_field<allocator_with_fit_mode::fit_mode>(_trusted_memory, fit_mode_offset) = mode;
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
