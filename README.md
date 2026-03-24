# Interacción con funciones de sockets

En la implementación del sistema Publisher–Broker–Subscriber se emplearon funciones de bajo nivel del lenguaje C para la comunicación mediante sockets.
A continuación, se describe la interacción del programa con cada función utilizada, indicando el archivo correspondiente.

## socket()

Se utiliza para crear los endpoints de comunicación.

### UDP — subscriber_udp.c
socketSubscriber = socket(AF_INET, SOCK_DGRAM, 0);

### TCP — publisher_tcp.c
socketPublisher = socket(AF_INET, SOCK_STREAM, 0);

Uso en el sistema:
- SOCK_STREAM corresponde a comunicación TCP (orientada a conexión)
- SOCK_DGRAM corresponde a comunicación UDP (no orientada a conexión)

## bind()

Se utiliza para asociar un socket a una dirección IP y un puerto.

### broker_udp.c
bind(socketUDPBroker, (struct sockaddr*)&direccionSocketUDPBroker, sizeof(direccionSocketUDPBroker));

### subscriber_udp.c
bind(socketSubscriber, (struct sockaddr*)&miDireccion, sizeof(miDireccion));

Uso en el sistema:
- El broker se asocia al puerto 5000
- El subscriber UDP utiliza un puerto dinámico para recibir mensajes

## listen()

Se utiliza en el broker TCP para escuchar conexiones entrantes.

### broker_tcp.c
listen(socketEscucha, 5);

Uso en el sistema:
- Permite que el broker acepte múltiples conexiones en espera

## accept()

Se utiliza para aceptar conexiones entrantes en el broker TCP.

### broker_tcp.c
int nuevoSocketCliente = accept(socketEscucha, (struct sockaddr*)&direccionSocketEscucha, (socklen_t*)&tamanioDireccionSocketEscucha);

Uso en el sistema:
- Cada cliente obtiene su propio socket de comunicación
- Permite manejar múltiples conexiones simultáneamente

## connect()

Se utiliza en clientes TCP para conectarse al broker.

### subscriber_tcp.c
connect(socketSubscriber, (struct sockaddr *)&direccionSocketBroker, sizeof(direccionSocketBroker));

### publisher_tcp.c
connect(socketPublisher, (struct sockaddr *)&direccionBroker, sizeof(direccionBroker));

Uso en el sistema:
- Establece conexión con el broker en 127.0.0.1:5000

## send() y sendto()

Se utilizan para enviar datos.

### publisher_tcp.c
send(socketPublisher, bufferMensaje, strlen(bufferMensaje), 0);

### publisher_udp.c
sendto(socketPublisher, bufferMensaje, strlen(bufferMensaje), 0, (struct sockaddr *)&direccionBroker, sizeof(direccionBroker));

### broker_udp.c
sendto(socketUDPBroker, bufferMensaje, strlen(bufferMensaje), 0, (struct sockaddr*)&listaSubscribers[i], sizeof(listaSubscribers[i]));

Uso en el sistema:
- El publisher envía eventos al broker
- El broker reenvía eventos a los subscribers

## recv(), read() y recvfrom()

Se utilizan para recibir datos.

### subscriber_tcp.c
int cantidadBytesLeidos = read(socketSubscriber, bufferMensaje, maxBytesFragmento);

### subscriber_udp.c
int cantidadBytesLeidos = recvfrom(socketSubscriber, bufferMensaje, maxBytesFragmento, 0, NULL, NULL);

### broker_udp.c
int bytesRecibidos = recvfrom(socketUDPBroker, bufferMensaje, maxBytesFragmento, 0, (struct sockaddr*)&direccionSocketCliente, &tamanioDireccionSocketCliente);

Uso en el sistema:
- El broker recibe eventos de los publishers
- Los subscribers reciben eventos del broker

## select()

Se utiliza en el broker TCP para manejar múltiples clientes.

### broker_tcp.c
select(mayorSocketVigilado + 1, &socketsVigilados, NULL, NULL, NULL);

Uso en el sistema:
- Detecta nuevas conexiones
- Detecta actividad en clientes
- Permite concurrencia sin hilos

## close()

Se utiliza para cerrar sockets.

### subscriber_tcp.c
close(socketSubscriber);

### broker_tcp.c
close(socketsClientes[i]);

Uso en el sistema:
- Libera recursos cuando un cliente se desconecta

## Conclusión

Las funciones de sockets fueron utilizadas de manera explícita y entendiendo su rol, lo cual nos ayudó a implementar la comunicación.

El sistema sigue la arquitectura: Publisher -> Broker -> Subscriber

y distingue correctamente entre:
- TCP: comunicación confiable con conexión establecida previamente
- UDP: comunicación rápida pero sin conexión establecida previamente
