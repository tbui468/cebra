# Cebra Scripting Language

## Building

```
cd cebra
mkdir build
cd build
cmake ..
cmake --build .
```

## Running with Script
```
./Cebra my_program.cbr
```

## Example Programs

```
//nth Fibonacci number
time := clock()
result := fib(20)

print("Time: " + (clock() - time) as string + "\n")
print("Result: " + result as string + "\n")

fib :: (i: int) -> (int) {
    if i < 2 {
        -> i
    }

    -> fib(i - 1) + fib(i - 2)
}
```

```
//stack data structure
Stack :: struct {
    values: List<string> = List<string>()
    count: int = 0
}

push :: (s: Stack, n: string) -> () {
    s.values[s.count] = n
    s.count = s.count + 1
}

pop :: (s: Stack) -> (string) {
    ret: string = s.values[s.count - 1]
    s.count = s.count - 1
    -> ret
}

stack := Stack()

push(stack, "one")
push(stack, "two")
push(stack, "three")

while stack.count > 0 {
    print(pop(stack) + "\n")
}
```

## Features

### Primitive Data Types
//int
//float
//string
//bool

a: int = 1 //explicitly declare type
b := 2  //type is deduced

### Structs
Coord :: struct {
    x: int = 1
    y: int = 2
}

c := Coord()
c.x = 25

d: Coord = nil //type cannot be deduced if value is nil

### Flow Control
a := 10
b := 20
if a < b {
    print(b as string + " is larger")
} else {
    print(a as string + " is larger or equal")
}

i := 0
while i < 5 {
    print(i)
    print("\n")
    i = i + 1
}

for i: int = 0, i < 10, i = i + 1 {
    print(i)
}

//when-is statements are similar to C-style switch statements
d := 0
e := 19
when d {
    is 2 {
        print("not this")
    }
    is e + d {
        print("not this")
    }
    is 0 {
        print("this")
    }
    else {
        print("not this")
    }
}

### Functions

add1 :: (a: int, b: int) -> () {
    print(a + b)
}

add1(1, 2)

add2 :: (a: int, b: int) -> (int) {
    -> a + b
}

a := add2(1, 2)
print(a)

sum_and_diff :: (a: int, b: int) -> (int, int) {
    -> a + b, a - b
}

a, b := sum_and_diff(1, 2)
print(a)
print("\n")
print(b)

### Modules
my_module.cbr

```
add :: (a: int, b: int) -> () {
    -> a + b
}
```

main.cbr

```
import my_module

a := add(1, 2)
print(a)
```

### Examples
See /examples for more examples

## Architecture

## Future Work
Rework List interface
Classes
Expand Standard Library
Improved Error Messages
