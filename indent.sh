#!/bin/bash

astyle --style=java --indent=tab --unpad-paren --pad-oper --pad-header --suffix=none --brackets=linux "$@"
