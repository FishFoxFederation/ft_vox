# Style guide {#styleguide}


FishFoxFederation's naming rules and style guide are mainly inspired from google's cpp [Style guide](https://google.github.io/styleguide/cppguide.html) 

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
