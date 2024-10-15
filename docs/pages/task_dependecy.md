# Task Dependency ( WIP ) {#taskDep}

This page is about the project to add a task depedency manager. That will give the ability to have task wait on eachother and even maybe have condition,
we would have clearly used [taskflow](https://github.com/taskflow/taskflow) if possible but we are at 42 so lets go recode a simpler version ourselves.

The new task manager will replace the current threadpool accessor and oversee every interaction related to tasks

### Needed features

- Static dependency graph construction
- easy push of just one task as most of our current code works like this
- exception safety
- easy waiting 
  - 1 graph = 1 thing to wait
  - possibility to group single tasks for waiting
- dynamic tasks dependency, ability to launch a single task with dependency to previously launched-but-not-waited-for tasks.
- work stealing scheduling algorithm
