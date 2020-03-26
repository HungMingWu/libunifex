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

#include <unifex/stream_concepts.hpp>

#include <functional>
#include <type_traits>

namespace unifex {
namespace _next_adapt {
  template <typename Stream, typename AdaptFunc>
  struct _stream {
    struct type {
      Stream innerStream_;
      AdaptFunc adapter_;

      friend auto tag_invoke(tag_t<next>, type& s)
        -> std::invoke_result_t<AdaptFunc&, next_sender_t<Stream>> {
        return std::invoke(s.adapter_, next(s.innerStream_));
      }

      friend auto tag_invoke(tag_t<cleanup>, type& s)
        -> cleanup_sender_t<Stream> {
        return cleanup(s.innerStream_);
      }
    };
  };
  template <typename Stream, typename AdaptFunc>
  using stream =
      typename _stream<
          std::remove_cvref_t<Stream>,
          std::remove_cvref_t<AdaptFunc>>::type;

} // namespace _next_adapt

namespace _next_adapt_cpo {
  inline constexpr struct _fn {
    template <typename Stream, typename AdaptFunc>
    auto operator()(Stream&& stream, AdaptFunc&& adapt) const {
      return _next_adapt::stream<Stream, AdaptFunc>{
          std::move(stream),
          std::move(adapt)};
    }
  } next_adapt_stream{};
} // namespace _next_adapt_cpo

using _next_adapt_cpo::next_adapt_stream;

} // namespace unifex
