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
#include "dl_library.h"

#include <dlfcn.h>
#include <link.h>

#include "logging.h"

namespace autocompat {

DlLibrary::DlLibrary(const std::filesystem::path &lib_path) {
    (void)this->open(lib_path, RTLD_LAZY | RTLD_LOCAL);
}

DlLibrary::~DlLibrary(void) { this->close(); }

DlLibrary::operator bool() const { return this->handle != nullptr; }

bool DlLibrary::open(const std::filesystem::path &lib_path, int flags) {
    log_trace("dlopen({})", lib_path);
    this->handle = ::dlopen(lib_path.c_str(), flags);
    if (this->handle == nullptr) {
        this->last_error.assign(::dlerror());
        log_trace("{}", this->last_error);
        return false;
    }
    return true;
}

void DlLibrary::close(void) {
    if (this->handle != nullptr) {
        log_trace("dlclose(handle)");
        if (::dlclose(this->handle) != 0) {
            this->last_error.assign(::dlerror());
            log_trace("{}", this->last_error);
        }
        this->handle = nullptr;
    }
}

std::string_view DlLibrary::get_path(void) const {
    if (this->handle == nullptr) {
        return {};
    }

    struct link_map *map = nullptr;
    log_trace("dlinfo(handle, RTLD_DI_LINKMAP)");
    if (::dlinfo(this->handle, RTLD_DI_LINKMAP, static_cast<void *>(&map)) !=
        0) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        const_cast<std::string &>(this->last_error).assign(::dlerror());
        log_trace("{}", this->last_error);
        return {};
    }

    return map->l_name;
}

const std::string &DlLibrary::get_last_error(void) const {
    return this->last_error;
}

// symbol names are almost always short enough to trigger SSO
// NOLINTNEXTLINE(performance-unnecessary-value-param)
void *DlLibrary::get_symbol_pointer(const std::string name) const {
    if (this->handle == nullptr) {
        return nullptr;
    }
    log_trace("dlsym({})", name);
    void *sym = ::dlsym(this->handle, name.c_str());
    if (sym == nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        const_cast<std::string &>(this->last_error).assign(::dlerror());
        log_trace("{}", this->last_error);
    }
    return sym;
}

} // namespace autocompat
