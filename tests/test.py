import time


def Fib(n):
    if n < 2:
        return n
    
    return Fib(n-1) + Fib(n-2)


t = time.time()
Fib(35)
print(time.time() - t)


