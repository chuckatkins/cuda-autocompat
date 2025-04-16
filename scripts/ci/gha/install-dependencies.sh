#!/bin/bash

echo "::group:: Determining packages to install"
packages=(build-essential cmake ninja-build)
if [ "${CC}" = "clang" ]
then
    packages+=(clang llvm)
fi
if [[ "${CXXFLAGS}" = *"-stdlib=libc++"* ]]
then
    packages+=(libc++-dev libc++abi-dev)
fi
if [ -n "${EXTRA_PACKAGES}" ]
then
    packages+=(${EXTRA_PACKAGES})
fi
echo "${packages[@]}"
echo "::endgroup::"

echo "::group:: Setting up Kitware APT repo"
codename=$(sed -n 's/^UBUNTU_CODENAME=\(.*\)/\1/p' /etc/os-release)
keyring=/usr/share/keyrings/kitware-archive-keyring.gpg

# Install the Kitware signing keys
if [ ! -f /usr/share/doc/kitware-archive-keyring/copyright ]
then
    curl \
        https://apt.kitware.com/keys/kitware-archive-latest.asc \
        2>/dev/null | \
    gpg --dearmor - | \
    tee ${keyring} >/dev/null
fi

# Install the Kitare apt repo
cat << EOF > /etc/apt/sources.list.d/kitware.list
deb [signed-by=${keyring}] https://apt.kitware.com/ubuntu/ ${codename} main
EOF
echo "::endgroup::"

echo "::group:: APT Update"
apt-get update
echo "::endgroup::"

echo "::group:: APT Install"
apt-get install -y ${packages[@]}
echo "::endgroup::"
