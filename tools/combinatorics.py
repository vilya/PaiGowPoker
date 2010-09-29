import operator

def factorial(n):
  return reduce(operator.mul, xrange(n, 1, -1), 1)


def choose(n, k):
  return factorial(n) / (factorial(n - k) * factorial(k))

