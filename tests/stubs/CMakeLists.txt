# Copyright 2025 Chuck Atkins and CUDA Auto-Compat contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include(AutoCompatStubUtils.cmake)

add_stub_driver(TARGET stub_driver_123 VERSION 1.2.3)
add_stub_driver(TARGET stub_driver_234 VERSION 2.3.4)
add_stub_driver(TARGET stub_driver_noerror VERSION 0 NOIMPL)
add_stub_driver(TARGET stub_driver_567 VERSION 5.6.7)

add_library(stub_driver_autocompat SHARED)
target_link_libraries(stub_driver_autocompat PRIVATE utils_version)
set_stub_driver_properties(TARGET stub_driver_autocompat)

add_stub_toolkit(TARGET stub_toolkit_345 VERSION 3.4.5)
add_stub_toolkit(TARGET stub_toolkit_456 VERSION 4.5.6 NOCOMPAT)

add_stub_other(stub_other_foo)
add_stub_other(stub_other_bar)
add_stub_other(stub_other_baz)
