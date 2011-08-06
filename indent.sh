#!/bin/bash

FILE=$1

astyle --style=java --indent=tab --unpad-paren --pad-oper --pad-header --suffix=none --brackets=linux $FILE
