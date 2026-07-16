import binascii
import socket
import secrets
import hashlib
import time

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 500*1024*1024)

r_f = open("/dev/urandom", "rb")
def send(msg, to):
    hasher = hashlib.blake2b(digest_size=32)
    hasher.update(msg)
    msg += hasher.digest()
    msg = bytes(msg)
    s.sendto(msg, to)
dt = 1/100000
ts = time.time_ns()
ns = 10**9
b = r_f.read(1500)
r_f.close()
for i in range(1000000):
    send(b, ("127.0.0.1", 8080))
    #print(len(b))
    ts += dt*ns
    while time.time_ns() < ts:
        pass

#for i in range(10):
#    s.recvfrom(1400)


