import binascii
import socket
import secrets
import time

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 500*1024*1024)
r_f = open("/dev/urandom", "rb")
def send(msg, to):
    msg = binascii.crc32(msg).to_bytes(4, byteorder='big') + msg
    msg = list(msg)
    #msg[0] = 255
    msg = bytes(msg)
    s.sendto(msg, to)
dt = 1/200000
ts = time.time_ns()
ns = 10**9
b = r_f.read(1024)
r_f.close()
for i in range(10):
    send(b, ("127.0.0.1", 8080))
    ts += dt*ns
    while time.time_ns() < ts:
        pass

#for i in range(10):
#    s.recvfrom(1400)

msg = b"Hello Nax"
msg = binascii.crc32(msg).to_bytes(4, byteorder='big') + msg
t = 209/(10**9)
print(1/t *1500 /1024/1024/1024)
#for i in msg:
#    print(i)
#print(binascii.hexlify(binascii.crc32(msg).to_bytes(4, byteorder='big')))
