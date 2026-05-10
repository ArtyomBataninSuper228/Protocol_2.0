from Base.protocol_core import *
num = 100
all = 0
errors = 0
#client = None
global client
print(threading.active_count())
for i in range(num):
            try:
                print(i)
                client = Connection("127.0.0.1", 5000, timeout=0.7)

                #client.timeout = 10
                #client.delay = 1*1024*1024/1280
                #b = secrets.token_bytes(1200)
                #for i in range(10*1024*1024//1280):
                #    client.add_packet(2, 0, b)
                #time.sleep(10)
                client.close()
            except Exception as e:
                print("ERRORRR", client.num_recieved_packets, client.is_started, client.evented, client.processed, threading.active_count())
                #exception(str(e))
                errors += 1
            all += 1
time.sleep(0.5)
print(f"Handshake test completed, all:{all}, errors:{errors}, Loss:{errors/all}")


