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

#include <unifex/sender_concepts.hpp>
#include <unifex/tag_invoke.hpp>

#include <type_traits>
#include <exception>

namespace unifex {
namespace _schedule {
  struct sender;
  inline constexpr struct _fn {
    template<bool>
    struct _impl {
      template <typename Scheduler>
      constexpr auto operator()(Scheduler&& s) const
          noexcept(is_nothrow_tag_invocable_v<_fn, Scheduler>)
          -> tag_invoke_result_t<_fn, Scheduler> {
        return tag_invoke(_fn{}, std::move(s));
      }
    };

    template <typename Scheduler>
    constexpr auto operator()(Scheduler&& s) const
        noexcept(is_nothrow_callable_v<
            _impl<is_tag_invocable_v<_fn, Scheduler>>, Scheduler>)
        -> callable_result_t<
            _impl<is_tag_invocable_v<_fn, Scheduler>>, Scheduler> {
      return _impl<is_tag_invocable_v<_fn, Scheduler>>{}(
          std::move(s));
    }

    constexpr sender operator()() const noexcept;
  } schedule{};

  template<>
  struct _fn::_impl<false> {
    template <typename Scheduler>
    constexpr auto operator()(Scheduler&& s) noexcept(
        noexcept(std::move(s).schedule()))
        -> decltype(std::move(s).schedule()) {
      return std::move(s).schedule();
    }
  };
} // namespace _schedule
using _schedule::schedule;

namespace _get_scheduler {
  inline constexpr struct _fn {
    template <typename Context>
    auto operator()(const Context &context) const noexcept
        -> tag_invoke_result_t<_fn, const Context &> {
      static_assert(is_nothrow_tag_invocable_v<_fn, const Context &>);
      static_assert(is_callable_v<
                    decltype(schedule),
                    tag_invoke_result_t<_fn, const Context &>>);
      return tag_invoke(*this, context);
    }
  } get_scheduler{};
} // namespace _get_scheduler
using _get_scheduler::get_scheduler;

struct _schedule::sender {
  template<
    template<typename...> class Variant,
    template<typename...> class Tuple>
  using value_types = Variant<Tuple<>>;

  template<template<typename...> class Variant>
  using error_types = Variant<std::exception_ptr>;

  template <
    typename Receiver,
    typename Scheduler =
      std::decay_t<callable_result_t<decltype(get_scheduler), const Receiver&>>,
    typename ScheduleSender = callable_result_t<decltype(schedule), Scheduler&>>
  friend auto tag_invoke(tag_t<connect>, sender, Receiver &&r)
      -> operation_t<ScheduleSender, Receiver> {
    auto scheduler = get_scheduler(std::as_const(r));
    return connect(schedule(scheduler), std::move(r));
  }
};

inline constexpr _schedule::sender _schedule::_fn::operator()() const noexcept {
  return {};
}

namespace _schedule_after {
  template<typename Duration>
  struct _sender {
    class type;
  };
  template<typename Duration>
  using sender = typename _sender<Duration>::type;

  inline constexpr struct _fn {
    template<bool>
    struct _impl {
      template <typename TimeScheduler, typename Duration>
      constexpr auto operator()(TimeScheduler&& s, Duration&& d) const
          noexcept(is_nothrow_tag_invocable_v<_fn, TimeScheduler, Duration>)
          -> tag_invoke_result_t<_fn, TimeScheduler, Duration> {
        return tag_invoke(*this, std::move(s), std::move(d));
      }
    };

    template <typename TimeScheduler, typename Duration>
    constexpr auto operator()(TimeScheduler&& s, Duration&& d) const
        noexcept(is_nothrow_callable_v<
            _impl<is_tag_invocable_v<_fn, TimeScheduler, Duration>>, TimeScheduler, Duration>)
        -> callable_result_t<
            _impl<is_tag_invocable_v<_fn, TimeScheduler, Duration>>, TimeScheduler, Duration> {
      return _impl<is_tag_invocable_v<_fn, TimeScheduler, Duration>>{}(
          std::move(s),
          std::move(d));
    }

    template<typename Duration>
    constexpr sender<Duration> operator()(Duration d) const {
      return sender<Duration>{std::move(d)};
    }
  } schedule_after{};

  template<>
  struct _fn::_impl<false> {
    template <typename TimeScheduler, typename Duration>
    constexpr auto operator()(TimeScheduler&& s, Duration&& d) const noexcept(
        noexcept(std::move(s).schedule_after(std::move(d))))
        -> decltype(std::move(s).schedule_after(std::move(d))) {
      return std::move(s).schedule_after(std::move(d));
    }
  };

  template<typename Duration>
  class _sender<Duration>::type {
  public:
    template<template<typename...> class Variant, template<typename...> class Tuple>
    using value_types = Variant<Tuple<>>;

    template<template<typename...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    explicit type(Duration d)
      : duration_(d)
    {}

  private:
    friend _fn;

    template<
      typename Receiver,
      typename Scheduler =
        std::decay_t<callable_result_t<decltype(get_scheduler), const Receiver&>>,
      typename ScheduleAfterSender =
        callable_result_t<_fn, Scheduler&, const Duration&>>
    friend auto tag_invoke(tag_t<connect>, const type& s, Receiver&& r)
        -> operation_t<ScheduleAfterSender, Receiver> {
      auto scheduler = get_scheduler(std::as_const(r));
      return connect(schedule_after(scheduler, std::as_const(s.duration_)), std::move(r));
    }

    Duration duration_;
  };
} // namespace _schedule_after
using _schedule_after::schedule_after;

namespace _schedule_at {
  inline constexpr struct _fn {
    template<bool>
    struct _impl {
      template <typename TimeScheduler, typename TimePoint>
      constexpr auto operator()(TimeScheduler&& s, TimePoint&& tp) const
          noexcept(is_nothrow_tag_invocable_v<_fn, TimeScheduler, TimePoint>)
          -> tag_invoke_result_t<_fn, TimeScheduler, TimePoint> {
        return tag_invoke(*this, std::move(s), std::move(tp));
      }
    };

    template <typename TimeScheduler, typename TimePoint>
    constexpr auto operator()(TimeScheduler&& s, TimePoint&& tp) const
        noexcept(is_nothrow_callable_v<
            _impl<is_tag_invocable_v<_fn, TimeScheduler, TimePoint>>, TimeScheduler, TimePoint>)
        -> callable_result_t<
            _impl<is_tag_invocable_v<_fn, TimeScheduler, TimePoint>>, TimeScheduler, TimePoint> {
      return _impl<is_tag_invocable_v<_fn, TimeScheduler, TimePoint>>{}(
          std::move(s),
          std::move(tp));
    }
  } schedule_at {};

  template<>
  struct _fn::_impl<false> {
    template <typename TimeScheduler, typename TimePoint>
    constexpr auto operator()(TimeScheduler&& s, TimePoint&& tp) const noexcept(
        noexcept(std::move(s).schedule_at(std::move(tp))))
        -> decltype(std::move(s).schedule_at(std::move(tp))) {
      return std::move(s).schedule_at(std::move(tp));
    }
  };
} // namespace _schedule_at
using _schedule_at::schedule_at;

namespace _now {
  inline constexpr struct _fn {
    template<bool>
    struct _impl {
      template <typename TimeScheduler>
      constexpr auto operator()(TimeScheduler&& s) const
          noexcept(is_nothrow_tag_invocable_v<_fn, TimeScheduler>)
          -> tag_invoke_result_t<_fn, TimeScheduler> {
        return tag_invoke(*this, std::move(s));
      }
    };

    template <typename TimeScheduler>
    constexpr auto operator()(TimeScheduler&& s) const
        noexcept(is_nothrow_callable_v<
            _impl<is_tag_invocable_v<_fn, TimeScheduler>>, TimeScheduler>)
        -> callable_result_t<
            _impl<is_tag_invocable_v<_fn, TimeScheduler>>, TimeScheduler> {
      return _impl<is_tag_invocable_v<_fn, TimeScheduler>>{}(
          std::move(s));
    }
  } now {};

  template<>
  struct _fn::_impl<false> {
    template <typename TimeScheduler>
    constexpr auto operator()(TimeScheduler&& s) const noexcept(
        noexcept(std::move(s).now()))
        -> decltype(std::move(s).now()) {
      return std::move(s).now();
    }
  };
} // namespace _now
using _now::now;

} // namespace unifex
