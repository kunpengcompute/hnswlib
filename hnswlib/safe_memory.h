/*
   Copyright 2026 Huawei Technologies Co., Ltd.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#pragma once
#include <cstring>
#include <stdexcept>

namespace SafeMemory {

template <typename D, typename S>
void CheckAndMemcpy(D *dest, size_t destBufferSize, const S *src, size_t srcBufferSize) {
    if (srcBufferSize > destBufferSize) {
        throw std::invalid_argument(
            "Memcpy failed: destBufferSize[" + std::to_string(destBufferSize) +
            "] should be >= srcBufferSize[" + std::to_string(srcBufferSize) + "]."
        );
    }
    if (dest == nullptr || (src == nullptr && srcBufferSize != 0)) {
        throw std::invalid_argument("Memcpy failed: null pointer detected");
    }
    memcpy(dest, src, srcBufferSize);
}

}  // namespace SafeMemory
