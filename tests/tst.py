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
    #msg += hasher.digest()
    msg = bytes(msg)
    s.sendto(msg, to)
dt = 1/20000
ts = time.time_ns()
ns = 10**9
b = r_f.read(1400)
r_f.close()
while 1:
    send(b, ("192.168.1.9", 8080))
    #print(len(b))
    ts += dt*ns
    while time.time_ns() < ts:
        pass

#for i in range(10):
#    s.recvfrom(1400)


