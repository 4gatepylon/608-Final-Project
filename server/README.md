# How this works
You try and output your react build output into index.html.

# Server Usae
3 endpoints:
1. POST => post some data into the database by providing velocity, acceleration, position (etc) in each axis (x/y)
2. GET (without any values in the html) => get something from the DB (for backwards compatbility)... returns same format as POST would expect.
3. GET (with ANY values in the html) => get a webpage (later, our react webpage)

# Images
Either the images should be all from other sites (places than can serve the images) or they should be SVGs or otherwise embeddable in a single HTML file.

# Decorators example
```
def hi(func):
    def wrapper(*args, **kwargs):
        return func(3, *args, **kwargs)
    return wrapper

@hi
def f(number1, number2):
    print("adding {} and {}".format(number1, number2))
    return number1 + number2
f(2)    # works
f(1, 2) # does not work
```