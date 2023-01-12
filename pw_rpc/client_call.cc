// Copyright 2021 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "pw_rpc/internal/client_call.h"

namespace pw::rpc::internal {

void ClientCall::CloseClientCall() {
  if (client_stream_open()) {
    CloseClientStreamLocked().IgnoreError();
  }
  UnregisterAndMarkClosed();
}

void UnaryResponseClientCall::HandleCompleted(
    ConstByteSpan response, Status status) PW_NO_LOCK_SAFETY_ANALYSIS {
  UnregisterAndMarkClosed();

  auto on_completed_local = std::move(on_completed_);

  // The lock is only released when calling into user code. If the callback is
  // wrapped, this on_completed is an internal function that expects the lock to
  // be held, and releases it before invoking user code.
  if (!proto_callbacks_are_wrapped()) {
    rpc_lock().unlock();
  }

  if (on_completed_local) {
    on_completed_local(response, status);
  }
}

void StreamResponseClientCall::HandleCompleted(Status status) {
  UnregisterAndMarkClosed();
  auto on_completed_local = std::move(on_completed_);
  rpc_lock().unlock();

  if (on_completed_local) {
    on_completed_local(status);
  }
}

}  // namespace pw::rpc::internal
