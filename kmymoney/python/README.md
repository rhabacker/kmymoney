# Experimental Python bindings

This directory contains an **experimental** SWIG module that exposes a tiny API
for creating a `MyMoneyTransaction` from Python.

## Build

```bash
cmake -S . -B build -DBUILD_PYTHON_BINDINGS=ON
cmake --build build --target kmymoney_python
```

## Example

```python
from kmymoney_python import TransactionBuilder

transaction = TransactionBuilder()
transaction.setMemo("Lunch")
transaction.setCommodity("USD")
transaction.setPostDate(2026, 4, 11)
transaction.addSplit("A000001", "-12.50", "Cash")
transaction.addSplit("A000002", "12.50", "Food")

assert transaction.splitCount() == 2
assert transaction.isBalanced()
```

The current binding is intentionally narrow and focuses on transaction
construction only.
