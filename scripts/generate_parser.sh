#!/usr/bin/env bash

python3 -m lark.tools.standalone assets/grammars/definitions.g > scripts/definitions/parser.py
python3 scripts/test_parse_definitions.py
