# Testing the Bindings

Run from the root directory

```
$ python setup.py build_ext --inplace
$ pytest
```

You can also build an exectuable for debugging the Confetti C bindings with

```
$ cmake -B build
$ cmake --build build --config Release
$ cmake --install build --config Release
```
