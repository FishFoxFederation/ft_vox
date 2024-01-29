# Code style guide {#codestyleguide}

[TOC]

## General naming rules

### Variables

**Snake case**

```cpp
int my_var_in_degrees_c;
```

### Classes

**Pascal case**: each word starts by an uppercase including the first letter
~~~cpp
class MyClass;
class MyOtherClass;
struct MyStruct;
~~~

Private Data members always start with m_
~~~cpp
class MyClass
{
private:
	int m_my_int;
};
~~~

### Functions

**Camel case** : each word starts by an uppercase except the first one

~~~cpp
void myFunction(int param1, int param2);
~~~

Function params use the same case as variables
~~~cpp
void myOtherFunction(int param1, int my_other_param);
~~~

## Class guidelines

- Every non static data member of a class should be private
- Default dtors and ctors MUST be deleted or implementend
- Every getters MUST have a const variant
- If you create an RAII class, you SHOULD declare move ctors
