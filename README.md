Description:

A method for linux multi-thread communication

[Howto]

There is a sample program for message queue demo.

Compile this demo:

copy main.c from $(SRCROOT)/samples/quickstart/ to $(SRCROOT)

gcc -o demo src/msgque.c src/msgque.h main.c -lpthread -Wall

run ./demo



History:

Feb 21/2017
Add a beautiful readme
Add core source code

