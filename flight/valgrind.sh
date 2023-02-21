#!/bin/bash

valgrind --sim-hints=lax-ioctls --suppressions=valgrind.suppress $@

