# CUDA Auto-Compat
CUDA Auto-Comapt is a shim library around the CUDA driver runtime libraries to
automatically detect and load the appropriate set of driver runtime libraries
for the current runtime environment.

## CUDA Forward Compatibility
The runtime components for the CUDA toolkit depend on both a runtime library
provided by the toolkit, `libcudart.so.1`, and a runtime library provided by the
driver, `libcuda.so.1`.  Since newer versions of the CUDA toolkit may depend on
APIs or features in newer driver versions that what may be installed the toolkit
provides a forward compatibility package called `cuda-compat` containing a set of
driver runtime libraries that can be used by applications built with a newer
toolkit version to run against an older driver version.  To use the forward
compatibility libraries they just need to be found by the dynamic runtime linker,
`ld.so`, before the runtime libraries provided by the installed driver.  Typically
this can be done by adding the forward compatibility install location (e.g.
`/usr/local/cuda-12.4/compat`) to `LD_LIBRARY_PATH` so they get found before the
driver provided libraries in system search path (e.g. `/usr/lib64` on RHEL /
Fedora or `/usr/lib/x86_64-linux-gnu` on Debian / Ubuntu).

See [Forward Compatibility](https://docs.nvidia.com/deploy/cuda-compatibility/index.html#forward-compatibility)
for more details regarding forward compatibility.


## What's wrong with just using `LD_LIBRARY_PATH`?
Which set of driver runtime libraries to use, driver provided or toolkit provided
forward compatibility, depends on the specific combination of driver toolkit
versions.  This means that any change in either of the two versions may require
a change in which set of libraries to use.  For example, upgrading the driver on
a system may no longer need the forward compatibility libraries from the installed
toolkit and keeping them in `LD_LIBRARY_PATH` will break applications when CUDA
is initialized.  Similarly, if a newer toolkit is installed without an accompanying
driver upgrade then not using it's forward compatibility libraries can result in
a similar breakage.


## How does CUDA Auto-Compat solve this?
The CUDA Auto-Compat library provides a small shim `libcuda.so.1` library that
searches the runtime loader for all `libcuda.so.1` driver runtime libraries, not
just the first one found, and queries their supported CUDA API version.  The set of
libraries providing the newest API version is then dynamically loaded with
`dlopen`.  All of this happens when the shim is initially loaded by the runtime
linker via a function with the `constructor` attribute before any symbols are
resolved so when the runtime linker later tries to resolve symbols from
`libcuda.so.1` they are resolved by the loaded implementation instead of the shim.

This allows for both the driver and toolkit versions to change while still ensuring
the appropriate set of driver runtime libraries are used, so long as the
Auto-Compat provided `libcuda.so.1` is found first.
