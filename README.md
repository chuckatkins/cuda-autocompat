# CUDA Auto-Compat

**CUDA Auto-Compat** is a dynamic loader shim that automatically detects and
loads the most appropriate version of the CUDA driver runtime libraries
(`libcuda.so.1`, etc.) based on the runtime environment.

It eliminates the need for fragile `LD_LIBRARY_PATH` hacks by dynamically
resolving the best available driver stack at load time.

## CUDA Forward Compatibility

The CUDA runtime consists of two key shared libraries:

-   `libcudart.so.1`: provided by the **CUDA toolkit**
-   `libcuda.so.1`: provided by the **GPU driver**

Since newer versions of the CUDA toolkit may depend on features found only in
newer drivers, NVIDIA provides a **forward compatibility mechanism** via the
`cuda-compat` package. This package includes a set of driver runtime libraries
that allow newer toolkits to run on older installed drivers.

To use forward compatibility, these libraries must be found **before** the
system's driver-provided versions during dynamic linking - typically by
prepending their directory (e.g. `/usr/local/cuda-12.4/compat`) to
`LD_LIBRARY_PATH`.

See the official NVIDIA docs on
[CUDA Forward Compatibility](https://docs.nvidia.com/deploy/cuda-compatibility/index.html#forward-compatibility)
for more details.

## What's wrong with just using `LD_LIBRARY_PATH`?

Whether you need to use the system-provided driver libraries or the toolkit's
forward-compatibility libraries depends on the _specific_ combination of
installed driver and toolkit versions.

This means:

-   Upgrading your driver may make the forward-compat libraries
    **unnecessary** - but leaving them in `LD_LIBRARY_PATH` can **break**
    applications.
-   Installing a newer toolkit _without_ a driver upgrade may **require**
    forward-compat libraries - and omitting them can break applications in a
    different way.

Maintaining `LD_LIBRARY_PATH` by hand becomes brittle, error-prone, and
incompatible with flexible environments.

## How does CUDA Auto-Compat solve this?

CUDA Auto-Compat dynamically scans the system for _all available_ driver runtime
libraries, determines which provide the newest supported CUDA API version, and
configures the dynamic linker to use those.

This guarantees that:

-   The best compatible driver stack is always used
-   You can safely change driver or toolkit versions without needing to update
    environment variables
-   Applications don’t need to know or care - it "just works"

## Implementation Details

CUDA Auto-Compat is designed to be **transparent to applications** - no source
changes or rebuilds are required.

It provides two runtime interfaces that integrate with the dynamic linker:

-   :detective: **RTLD-AUDIT**: Audit Library (`libcuda_autocompat_audit.so.0`)
    Intercepts library loading via the `LD_AUDIT` mechanism and redirects
    specific CUDA driver libraries at runtime.

    Use via:

         `LD_AUDIT=/path/to/autocompat/lib64/libcuda_autocompat_audit.so.0`

-   :jigsaw: **GNU IFUNC**: IFUNC Shim Library (`libcuda.so.1`) A stub library
    using GNU `ifunc` resolvers to select the appropriate real driver libraries
    at symbol resolution time.

    Use via:

        `LD_LIBRARY_PATH=/path/to/autocompat/lib64:$LD_LIBRARY_PATH`

-   :mag: **HELPER EXECUTABLE**: Search Helper
    (`libexec/cuda_autocompat_search`) A statically linked C++ executable that
    performs all filesystem traversal and version selection logic. It is invoked
    automatically by both the RTLD-AUDIT and IFUNC libraries.

    NOTE: This is _not intended to be invoked directly_ by user applications.
