The main idea of project - create new protocol to replace TCP 
in networks with high loss, jitter and other negative phenomena.

Main "engine" of protocol is physical channel.

![Channel structure](documentation/images/Channels.drawio.png)


One connection may include many physical channels.
Every channel may be launched on different ports or
on different interfaces.
That structure supports using LTE and WI-FI or some 
modems at the same time.

Your application will operate with logical channels.
Logical channel may work in "TCP-like" mode or in 
"Block operating" mode.

In "TCP-like" mode logic channel will pretend TCP socket with
"send" and "recv" methods. Connection will automatically slice
your data to blocks and share it among physical channels.

In "Block operating" mode logic channel receives blocks from 
application and sends it with free or reserved physical 
channel. The most efficient using of "Block" mode is video
streaming, where 1 block = 1 frame.



Safety:
- Хэндшейк
1. Клиент передаёт серверу свой открытый ML-KEM ключ;
2. Сервер генерирует симметричный ключ для cha cha 20,
    шифрует его открытым ключём клиента и возвращает клиенту;
3. Клиент принимает ключ шифрования - вся последующая
передача данных проходит с шифрованием chacha20 Poly1305;
4. Сервер передаёт клиенту свой сертификат. Клиент шифрует им новый ключ
5. Сервер получает от клиента новый ключ шифрования. Поднимается
основной канал шифрования.
6. Сервер передаёт клиенту новый id и salt. Соль обновляется каждую секунду

- Формирование пакета:
1. Генерируется 24 байта nonce;
2. Шифрование данных ChaCha20 Poly1305 (с использованием 24 байт Nonce);
3. Хэш(id + salt) 
4. ID xor nonce 
 - Структура пакета: 
1) ID xor nonce
2) nonce
3) encoded data
    





