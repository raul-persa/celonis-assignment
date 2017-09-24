#!/usr/bin/env python
#
#   Hello World client in Python
#   Connects REQ socket to tcp://localhost:5555
#   Sends "Hello" to server, expects "World" back
#
import struct
import zmq

context = zmq.Context()

#  Socket to talk to server
print("Connecting to hello world server...")
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

def put(socket, key, value):
    print(len(key))
    bytes = struct.pack("<cQ%ds%ds"  % (len(key), len(value),), 'p', len(key), key, value);
    print(bytes)
    socket.send(bytes)
    rply = socket.recv()
    print("Received reply %s [ %s ]" % (request, rply))

def get(socket, key):
    bytes = struct.pack("<c%ds" % (len(key),), 'g', key);
    print(bytes)
    socket.send(bytes)
    rply = socket.recv()
    print("Received reply %s [ %s ]" % (request, rply))
    return rply

def delete(socket, key):
    bytes = struct.pack("<c%ds" % (len(key),), 'd', key);
    print(bytes)
    socket.send(bytes)
    rply = socket.recv()
    print("Received reply %s [ %s ]" % (request, rply))

#  Do 10 requests, waiting each time for a response
for request in range(10):
    print("Sending request %s ..." % request)
    put (socket, b"Hello", b"N00b")
    get (socket, b"Hello")
    delete (socket, b"Hello")