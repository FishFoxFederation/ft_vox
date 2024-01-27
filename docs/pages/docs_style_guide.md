# Docs Style guide {#docstyleguide}

[TOC]

## Intro {#intro}


Our documentation it generated using [doxygen](https://www.doxygen.nl/) as well as a github action to deploy it on github pages

Doxygen gives you the power to generate documentation based on special tagged comments

You can also create custom pages, a lot of different languages are supported but we chose to only use Markdown to create custom pages

## How to create a page 

### Useful Links

- [Doxygen's markdown doc](https://www.doxygen.nl/manual/markdown.html)
- [Markdown cheatsheet](https://github.com/lifeparticle/Markdown-Cheatsheet)

### File creation

Create a file with the .md suffix and put it in the docs/pages dir,

### Code blocks

You can create code blocks using 3 backticks :  \`

````
```
this is a code block
```
````

```
this is a code block
```

You SHOULD do syntax highlighting like this :
````
```cpp
class MySyntaxHighlightedClass;
```
````

```cpp
class MySyntaxHighlightedClass;
```

Doxygen doesnt support any other way

## How to document code

### Useful links

- [VScode auto doxygen snippet extension](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen)

You can put tags /** */ before a function a variable or a class
And use Doxygen's syntax and tags to document it

eg :
````cpp
/* @brief a simple function
 * 
 * @param param1 an int, MUST always be 42
 * @param param2 a void * actually it is not used anymore set it to NULL
 * @return an int, always 42
 */
int myIntFunction(int param1, void * param2);
````

wich gives :

[function doc](#myIntFunction)
