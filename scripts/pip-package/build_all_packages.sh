#!/bin/bash

PLATFORM="manylinux2014_x86_64"

chmod +x ./package_tar.sh
rm -rf wheelhouse graphflowdb.tar.gz && ./package_tar.sh
mkdir wheelhouse

# Build wheels, excluding pypy platforms
for PYBIN in /opt/python/*/bin; do
    if [[ $PYBIN == *"pypy"* ]]; then
        continue
    fi
    echo "Building wheel for $PYBIN..."
    "${PYBIN}/pip" install -r ../../tools/python_api/requirements_dev.txt
    "${PYBIN}/pip" wheel graphflowdb.tar.gz --no-deps -w wheelhouse/
done

for whl in wheelhouse/*.whl; do
    auditwheel repair $whl --plat "$PLATFORM" -w wheelhouse/
done
