#!/usr/bin/env bash
# Delegation wrapper: delegates to modular test suite runner
exec bash "$(dirname "$0")/run_tests.sh" "$@"
