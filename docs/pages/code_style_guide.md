# Code style guide {#codestyleguide}

[TOC]

## General naming rules

### Classes

Mixed case with a cap at every word start
~~~cpp
class MyClass;
class MyOtherClass;
struct MyStruct;
~~~

### Functions

Mixed case with a cap at every word start except the first word

Function params use the same case
~~~cpp
void myFunction(int param1, int param2);
void myOtherFunction(int param1, int myOtherParam);
~~~

### Variables

Snake case

```cpp
int my_var_in_degrees_c;
```

## Class guidelines

- Every non static data member of a class should be private
- Default dtors and ctors MUST be deleted or implementend
- Every getters MUST have a const variant
- If you create an RAII class, you SHOULD declare move ctors
