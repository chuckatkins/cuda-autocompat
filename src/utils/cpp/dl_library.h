/* Copyright 2025 Chuck Atkins and CUDA Auto-Compat contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef CUDA_AUTOCOMPAT_SEARCH_DL_LIBRARY_H
#define CUDA_AUTOCOMPAT_SEARCH_DL_LIBRARY_H

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

namespace autocompat {

class DlLibrary {
  public:
    DlLibrary(const DlLibrary &) = delete;
    DlLibrary &operator=(DlLibrary &) = delete;
    DlLibrary(DlLibrary &&) = delete;
    DlLibrary &operator=(DlLibrary &&) = delete;

    DlLibrary(void) = default;
    explicit DlLibrary(const std::filesystem::path &lib_path);
    ~DlLibrary(void);

    explicit operator bool() const;

    bool open(const std::filesystem::path &lib_path, int flags);

    void close(void);

    std::string_view get_path(void) const;

    const std::string &get_last_error(void) const;

    template <typename T>
    const T *get_data_symbol(const std::string name) const;

    template <typename TReturn, typename... TArgs>
    auto get_function_symbol(const std::string name) const;

  private:
    void *get_symbol_pointer(const std::string name) const;

    void *handle = nullptr;
    std::string last_error;
};

template <typename T>
const T *DlLibrary::get_data_symbol(const std::string name) const {
    return static_cast<T *>(this->get_symbol_pointer(name));
}

template <typename TReturn, typename... TArgs>
auto DlLibrary::get_function_symbol(const std::string name) const {
    const union {
        void *ptr;
        TReturn (*fptr)(TArgs...);
    } symbol = {.ptr = this->get_symbol_pointer(name)};

    return std::function<TReturn(TArgs...)>{symbol.fptr};
}
} // namespace autocompat
#endif // CUDA_AUTOCOMPAT_SEARCH_DL_LIBRARY_H
