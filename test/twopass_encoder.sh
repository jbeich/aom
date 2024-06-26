#!/bin/sh
## Copyright (c) 2016, Alliance for Open Media. All rights reserved.
##
## This source code is subject to the terms of the BSD 2 Clause License and
## the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
## was not distributed with this source code in the LICENSE file, you can
## obtain it at www.aomedia.org/license/software. If the Alliance for Open
## Media Patent License 1.0 was not distributed with this source code in the
## PATENTS file, you can obtain it at www.aomedia.org/license/patent.
##
## This file tests the libaom twopass_encoder example. To add new tests to this
## file, do the following:
##   1. Write a shell function (this is your test).
##   2. Add the function to twopass_encoder_tests (on a new line).
##
. $(dirname $0)/tools_common.sh

# Environment check: $YUV_RAW_INPUT is required.
twopass_encoder_verify_environment() {
  if [ ! -e "${YUV_RAW_INPUT}" ]; then
    echo "Libaom test data must exist in LIBAOM_TEST_DATA_PATH."
    return 1
  fi
}

# Runs twopass_encoder using the codec specified by $1 with a frame limit of
# 100.
twopass_encoder() {
  local encoder="$(aom_tool_path twopass_encoder)"
  local codec="$1"
  local output_file="${AOM_TEST_OUTPUT_DIR}/twopass_encoder_${codec}.ivf"
  local limit=7

  if [ ! -x "${encoder}" ]; then
    elog "${encoder} does not exist or is not executable."
    return 1
  fi

  eval "${AOM_TEST_PREFIX}" "${encoder}" "${codec}" "${YUV_RAW_INPUT_WIDTH}" \
      "${YUV_RAW_INPUT_HEIGHT}" "${YUV_RAW_INPUT}" "${output_file}" "${limit}" \
      ${devnull} || return 1

  [ -e "${output_file}" ] || return 1
}

twopass_encoder_av1() {
  if [ "$(av1_encode_available)" = "yes" ]; then
    twopass_encoder av1 || return 1
  fi
}

twopass_encoder_tests="twopass_encoder_av1"

run_tests twopass_encoder_verify_environment "${twopass_encoder_tests}"
