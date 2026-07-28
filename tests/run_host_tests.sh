#!/usr/bin/env sh
set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="${TMPDIR:-/tmp}/esp32mqttclient-host-tests"
compiler="${CXX:-c++}"

mkdir -p "$build_dir"

run_for_idf_version() {
    idf_label="$1"
    idf_version="$2"
    test_binary="$build_dir/host_tests_$idf_label"

    "$compiler" \
        -std=c++11 \
        -Wall \
        -Wextra \
        -Werror \
        -pedantic \
        -fno-rtti \
        -pthread \
        -DESP_IDF_VERSION="$idf_version" \
        -I"$repo_root/tests/fakes" \
        -I"$repo_root/src" \
        "$repo_root/tests/host_tests.cpp" \
        -o "$test_binary"

    printf '%s compatibility branch:\n' "$idf_label"
    "$test_binary"
}

run_for_idf_version "ESP-IDF 4.4.6" 263174
run_for_idf_version "ESP-IDF 5.3" 328448
