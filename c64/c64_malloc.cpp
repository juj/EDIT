#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct region
{
  uint16_t size;
  region *next;
};
#define REGION_END(reg) (region*)((uintptr_t)(reg) + (reg)->size)

extern "C" char __heap_start;
static region *first_free_region = (region*)&__heap_start;
static region *unsorted_free_region = 0;

#define MALLOC_HEAP_CEILING 0xC700u

__attribute__((constructor(99999)))
static void initialize_malloc()
{
  region *r = (region*)first_free_region;
  r->size = MALLOC_HEAP_CEILING - (uint16_t)&__heap_start;
  r->next = 0;
}

static bool linked_list_has_a_cycle(region *r)
{
  if (!r) return false;
  for(region *r2 = r->next; r2;)
  {
//    printf("A R: 0X%X, R2: 0X%X\n", (uintptr_t)r, (uintptr_t)r2);
    r = r->next;
    r2 = r2->next;
//    printf("B R: 0X%X, R2: 0X%X\n", (uintptr_t)r, (uintptr_t)r2);
    if (!r2) return false;
    if (r2 == r) return true;
    r2 = r2->next;    
//    printf("C R: 0X%X, R2: 0X%X\n", (uintptr_t)r, (uintptr_t)r2);
    if (r2 == r) return true;
  }
  return false;
}

static bool regions_overlap(region *r1, region *r2)
{
  if (!r1 || !r2) return false;
  return r2 < REGION_END(r1) && r1 < REGION_END(r2);
}

static void assert_check_region_consistency(region *r)
{
  assert(r->size >= 4);
  assert(r->next != r);
  assert(r->next != first_free_region || first_free_region == 0);
  assert(r->next == 0 || r->next != unsorted_free_region);
  assert((uintptr_t)r < MALLOC_HEAP_CEILING);
  assert(r->size < MALLOC_HEAP_CEILING);
  assert((uintptr_t)r + r->size <= MALLOC_HEAP_CEILING);
  assert(!regions_overlap(r, r->next));
}

static bool ptr_is_contained_in_free_heap(void *ptr)
{
  for(region *r = first_free_region; r; r = r->next)
    if (r <= ptr && ptr < REGION_END(r))
      return true;
  for(region *r = unsorted_free_region; r; r = r->next)
    if (r <= ptr && ptr < REGION_END(r))
      return true;
  return false;
}

extern "C" void malloc_verify_heap_consistency()
{
  assert(!linked_list_has_a_cycle(first_free_region));
  assert(!linked_list_has_a_cycle(unsorted_free_region));
  for(region *r = first_free_region; r; r = r->next)
  {
    assert_check_region_consistency(r);
    assert(!r->next || r->next > r); // Check that successor has a higher address than predecessor (sorted in ascending address order)
  }
  for(region *r = unsorted_free_region; r; r = r->next)
    assert_check_region_consistency(r);
}

extern "C" void *malloc(size_t bytes)
{
//  printf("A\n");
  malloc_verify_heap_consistency();
  if (bytes < 2) bytes = 2; // Must allocate at least two payload bytes
  bytes += 2; // and each memory allocation will have two byte overhead, so account for that early on
  if (bytes < 2)
  {
    assert(false);
    return 0; // On size overflow, abort allocation
  }
  for(region *r = first_free_region, *prev = 0; r; prev = r, r = r->next)
    if (bytes <= r->size)
    {
      // This region can fit the allocation. Decide if we should split the region in two,
      // or if the region should be returned in full to the user.
      size_t regionLeftOver = r->size - bytes;
      if (regionLeftOver >= 4) // If we can fit a new region at the end of the asked allocation size, split the region in two
      {
        region *newRegion = (region*)((uintptr_t)r + bytes);
        newRegion->size = regionLeftOver;
        newRegion->next = r->next;
        if (prev) prev->next = newRegion;
        else first_free_region = newRegion;
        r->size = bytes;
      }
      else // We cannot fit a new region, so return the whole region.
      {
        if (prev) prev->next = r->next;
        else first_free_region = r->next;
      }
//  printf("B\n");
  malloc_verify_heap_consistency();
  assert(!ptr_is_contained_in_free_heap(r));
  assert(!ptr_is_contained_in_free_heap((void*)((uintptr_t)r + 2)));
      return (void*)((uintptr_t)r + 2);
    }
  return 0;
}

extern "C" void malloc_dump_heap()
{
  for(region *r = first_free_region; r; r = r->next)
    printf("R: 0X%X -> 0X%X, NEXT: 0X%X\n", (uintptr_t)r, (uintptr_t)r + r->size, (uintptr_t)r->next);
  for(region *r = unsorted_free_region; r; r = r->next)
    printf("U: 0X%X -> 0X%X, NEXT: 0X%X\n", (uintptr_t)r, (uintptr_t)r + r->size, (uintptr_t)r->next);
}

extern "C" size_t malloc_usable_size(void *ptr)
{
  return *(size_t*)((uintptr_t)ptr - 2);
}

extern "C" void *realloc(void *ptr, size_t bytes)
{
  if (!ptr) return malloc(bytes);
  if (!bytes) { free(ptr); return 0; }

  // TODO optimize with a slow linear search?
  void *newPtr = malloc(bytes);
  if (!newPtr) return 0;
  memcpy(newPtr, ptr, malloc_usable_size(ptr));
  free(ptr);
  return newPtr;
}

extern "C" void malloc_defrag(uint8_t times)
{
//  malloc_dump_heap();
//  malloc_verify_heap_consistency();
  while(times--)
  {
//    printf("DEFRAG %u\n", (uint16_t)times);
    region *r = unsorted_free_region;
    if (!r) return;
    unsorted_free_region = r->next;

    if (!first_free_region) { first_free_region = r; continue; }

    if (r < first_free_region)
    {
      if (REGION_END(r) == first_free_region)
      {
        r->size += first_free_region->size;
        r->next = first_free_region->next;
      }
      else
        r->next = first_free_region;
      first_free_region = r;
      continue;
    }

    for(region *prev = first_free_region, *next = prev->next; prev; prev = next, next = next->next)
    {
      if (REGION_END(prev) == r)
      {
        prev->size += r->size;
        if (REGION_END(prev) == next)
        {
          prev->size += next->size;
          prev->next = next->next;
        }
        break;
      }
      if (r < next)
      {
        if (REGION_END(r) == next)
        {
          r->size += next->size;
          prev->next = r;
          r->next = next->next;
          break;
        }
        prev->next = r;
        r->next = next;
        break;
      }
      if (!next)
      {
        prev->next = r;
        break;
      }
    }
  }
//  malloc_dump_heap();
//  malloc_verify_heap_consistency();
}

extern "C" void malloc_add_free_block(void *ptr, size_t size)
{
  region *r = (region*)ptr;
  r->size = size;
  r->next = unsorted_free_region;
  unsorted_free_region = r;
  malloc_defrag(1);
}

extern "C" void free(void *ptr)
{
  if (!ptr) return;

//  printf("FREE(0X%X)\n", (uint16_t)ptr);
//  malloc_dump_heap();
//  malloc_verify_heap_consistency();
  assert(!ptr_is_contained_in_free_heap(ptr));

  region *r = (region*)((uintptr_t)ptr - 2);
  assert(!ptr_is_contained_in_free_heap(r));
  r->next = unsorted_free_region;
  unsorted_free_region = r;

  malloc_defrag(1);
//  printf("D\n");
//  malloc_dump_heap();
  assert(ptr_is_contained_in_free_heap(r));
  malloc_verify_heap_consistency();
}

#include <new>

namespace std
{
  const nothrow_t nothrow;
  static new_handler current_new_handler = nullptr;
  new_handler get_new_handler() noexcept { return current_new_handler; }
  new_handler set_new_handler(new_handler new_p) noexcept
  {
    new_handler old = current_new_handler;
    current_new_handler = new_p;
    return old;
  }
}

void *operator new(std::size_t bytes, const std::nothrow_t &) noexcept
{
  void *ptr;
  for(ptr = malloc(bytes); !ptr && std::get_new_handler(); std::get_new_handler()()) ;
  return ptr;
}

void *operator new(std::size_t bytes) { return operator new(bytes, std::nothrow); } // TODO: throw std::bad_alloc
void *operator new[](std::size_t size) { return operator new(size); }
void *operator new[](std::size_t count, const std::nothrow_t &) noexcept { return operator new(count, std::nothrow); }
void *operator new(std::size_t, void *ptr) { return ptr; }
void *operator new[](std::size_t, void *ptr) { return ptr; }
void operator delete(void *ptr) noexcept { free(ptr); }
void operator delete[](void *ptr) noexcept { operator delete(ptr); }
