/*
 * Copyright 2019-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <unifex/detail/atomic_intrusive_queue.hpp>
#include <unifex/detail/intrusive_queue.hpp>
#include <unifex/receiver_concepts.hpp>
#include <unifex/sender_concepts.hpp>
#include <unifex/tag_invoke.hpp>

namespace unifex {

class async_mutex {
  class lock_sender;

public:
  async_mutex() noexcept;
  async_mutex(const async_mutex &) = delete;
  async_mutex(async_mutex &&) = delete;
  ~async_mutex();

  async_mutex &operator=(const async_mutex &) = delete;
  async_mutex &operator=(async_mutex &&) = delete;

  [[nodiscard]] bool try_lock() noexcept;

  [[nodiscard]] lock_sender async_lock() noexcept;

  void unlock() noexcept;

private:
  struct waiter_base {
    void (*resume_)(waiter_base *) noexcept;
    waiter_base *next_;
  };

  class lock_sender {
  public:
    template <template <typename...> class Variant,
              template <typename...> class Tuple>
    using value_types = Variant<Tuple<>>;

    template <template <typename...> class Variant>
    using error_types = Variant<>;

  private:
    friend async_mutex;

    explicit lock_sender(async_mutex &mutex) noexcept
      : mutex_(mutex) {}

    lock_sender(const lock_sender &) = delete;
    lock_sender(lock_sender &&) = default;

    template <typename Receiver>
    struct _op {
      class type : waiter_base {
        friend lock_sender;

        template <typename Receiver2>
        explicit type(async_mutex &mutex, Receiver2 &&r) noexcept
            : mutex_(mutex), receiver_(std::move(r)) {
          this->resume_ = [](waiter_base * self) noexcept {
            type &op = *static_cast<type *>(self);
            unifex::set_value(std::move(op.receiver_));
          };
        }

        type(type &&) = delete;

        friend void tag_invoke(tag_t<start>, type &op) noexcept {
          if (!op.mutex_.try_enqueue(&op)) {
            // Failed to enqueue because we acquired the lock
            // synchronously. Invoke the continuation inline
            // without type-erasure here.
            set_value(std::move(op.receiver_));
          }
        }

        async_mutex &mutex_;
        Receiver receiver_;
      };
    };
    template <typename Receiver>
    using operation = typename _op<std::remove_cvref_t<Receiver>>::type;

    template <typename Receiver>
    friend operation<Receiver>
    tag_invoke(tag_t<connect>, lock_sender &&s, Receiver &&r) noexcept {
      return operation<Receiver>{s.mutex_, std::move(r)};
    }

    async_mutex &mutex_;
  };

  // Attempt to enqueue the waiter object to the queue.
  // Returns true if successfully enqueued, false if it was not enqueued because
  // the lock was acquired synchronously.
  bool try_enqueue(waiter_base *waiter) noexcept;

  atomic_intrusive_queue<waiter_base, &waiter_base::next_> atomicQueue_;
  intrusive_queue<waiter_base, &waiter_base::next_> pendingQueue_;
};

inline async_mutex::lock_sender async_mutex::async_lock() noexcept {
  return lock_sender{*this};
}

inline bool async_mutex::try_lock() noexcept {
  return atomicQueue_.try_mark_active();
}

} // namespace unifex
