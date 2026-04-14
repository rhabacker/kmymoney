#!/usr/bin/python3.11

import sys
sys.path.insert(0, '@CMAKE_BINARY_DIR@/kmymoney/python')
sys.path.insert(0, '@CMAKE_BINARY_DIR@/lib')

from kmymoney_python import TransactionBuilder

transaction = TransactionBuilder()
transaction.setMemo("Lunch")
transaction.setCommodity("USD")
transaction.setPostDate(2026, 4, 11)
transaction.addSplit("A000001", "-12.50", "Cash")
transaction.addSplit("A000002", "12.50", "Food")

assert transaction.splitCount() == 2
assert transaction.isBalanced()
