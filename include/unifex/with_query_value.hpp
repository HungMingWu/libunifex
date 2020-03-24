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

#include <unifex/get_allocator.hpp>
#include <unifex/receiver_concepts.hpp>
#include <unifex/sender_concepts.hpp>
#include <unifex/tag_invoke.hpp>

namespace unifex {
namespace _with_query_value {

template<typename CPO, typename Value, typename Receiver>
struct _receiver_wrapper {
  class type;
};
template<typename CPO, typename Value, typename Receiver>
using receiver_wrapper = typename _receiver_wrapper<CPO, Value, Receiver>::type;

template<typename CPO, typename Value, typename Receiver>
class _receiver_wrapper<CPO, Value, Receiver>::type {
 public:
  template <typename Receiver2>
  explicit type(Receiver2 &&receiver, const Value& val)
    : receiver_(std::move(receiver))
    , val_(&val) {}

 private:
  friend const Value &tag_invoke(CPO, const type &r) noexcept {
    return *r.val_;
  }

  template <typename OtherCPO, typename... Args>
  friend auto tag_invoke(OtherCPO cpo, const type &r, Args &&... args)
      noexcept(is_nothrow_callable_v<OtherCPO, const Receiver &, Args...>)
      -> callable_result_t<OtherCPO, const Receiver &, Args...> {
    return std::move(cpo)(std::as_const(r.receiver_), std::move(args)...);
  }

  template <typename OtherCPO, typename... Args>
  friend auto tag_invoke(OtherCPO cpo, type &r, Args &&... args)
      noexcept(is_nothrow_callable_v<OtherCPO, Receiver &, Args...>)
      -> callable_result_t<OtherCPO, Receiver &, Args...> {
    return std::move(cpo)(r.receiver_, std::move(args)...);
  }

  template <typename OtherCPO, typename... Args>
  friend auto tag_invoke(OtherCPO cpo, type &&r, Args &&... args)
      noexcept(is_nothrow_callable_v<OtherCPO, Receiver, Args...>)
      -> callable_result_t<OtherCPO, Receiver, Args...> {
    return std::move(cpo)(std::move(r.receiver_), std::move(args)...);
  }

  Receiver receiver_;
  const Value* val_;
};

template <typename CPO, typename Value, typename Sender, typename Receiver>
struct _op {
  class type;
};
template <typename CPO, typename Value, typename Sender, typename Receiver>
using operation = typename _op<CPO, Value, Sender, std::remove_cvref_t<Receiver>>::type;

template <typename CPO, typename Value, typename Sender, typename Receiver>
class _op<CPO, Value, Sender, Receiver>::type {
 public:
  template <typename Receiver2, typename Value2>
  explicit type(Sender &&sender, Receiver2 &&receiver, Value2 &&value)
    : value_(std::move(value))
    , innerOp_(
          connect(std::move(sender),
                  receiver_wrapper<CPO, Value, Receiver>{
                      std::move(receiver), value_})) {}

  void start() & noexcept {
    unifex::start(innerOp_);
  }

 private:
  UNIFEX_NO_UNIQUE_ADDRESS Value value_;
  /*UNIFEX_NO_UNIQUE_ADDRESS*/
  operation_t<Sender, receiver_wrapper<CPO, Value, Receiver>> innerOp_;
};

template <typename CPO, typename Value, typename Sender>
struct _sender {
  class type;
};
template <typename CPO, typename Value, typename Sender>
using sender =
    typename _sender<CPO, std::decay_t<Value>, std::remove_cvref_t<Sender>>::type;

template <typename CPO, typename Value, typename Sender>
class _sender<CPO, Value, Sender>::type {
public:
  template <template <typename...> class Variant,
            template <typename...> class Tuple>
  using value_types = typename Sender::template value_types<Variant, Tuple>;

  template <template <typename...> class Variant>
  using error_types = typename Sender::template error_types<Variant>;

  template <typename Sender2, typename Value2>
  explicit type(Sender2 &&sender, Value2 &&value)
    : sender_(std::move(sender)), value_(std::move(value)) {}

  template <typename Receiver>
  operation<CPO, Value, Sender, Receiver> connect(Receiver &&receiver) && {
    return operation<CPO, Value, Sender, Receiver>{
        std::move(sender_), std::move(receiver), std::move(value_)};
  }

  template <typename Receiver>
  operation<CPO, Value, Sender &, Receiver> connect(Receiver &&receiver) & {
    return operation<CPO, Value, Sender &, Receiver>{
        sender_, std::move(receiver), value_};
  }

  template <typename Receiver>
  operation<CPO, Value, const Sender &, Receiver> connect(Receiver &&receiver) const & {
    return operation<CPO, Value, const Sender &, Receiver>{
        sender_, std::move(receiver), value_};
  }

private:
  Sender sender_;
  Value value_;
};
} // namespace _with_query_value

namespace _with_query_value_cpo {
  inline constexpr struct _fn {
    template <typename Sender, typename CPO, typename Value>
    _with_query_value::sender<CPO, Value, Sender>
    operator()(Sender &&sender, CPO, Value &&value) const {
      static_assert(
          std::is_empty_v<CPO>,
          "with_query_value() does not support stateful CPOs");
      return _with_query_value::sender<CPO, Value, Sender>{
          std::move(sender),
          std::move(value)};
    }
  } with_query_value {};
} // namespace _with_query_value_cpo
using _with_query_value_cpo::with_query_value;

} // namespace unifex
