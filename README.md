# Interacción con funciones de sockets

En la implementación del sistema Publisher–Broker–Subscriber se emplearon funciones de bajo nivel del lenguaje C para la comunicación mediante sockets.
A continuación, se describe la interacción del programa con cada función utilizada, indicando el archivo correspondiente y explicando detalladamente los parámetros utilizados.

Además, se documentan explícitamente las librerías utilizadas, cumpliendo con el requisito de especificar punto a punto la interacción con cada una.

--------------------------------------------------

# Librerías utilizadas

## <stdio.h>
Se utiliza para entrada y salida por consola.

Funciones usadas:
- printf(): mostrar mensajes en consola.
- fgets(): leer mensajes desde teclado.

--------------------------------------------------

## <string.h>
Se utiliza para manipulación de cadenas.

Funciones usadas:
- strlen(): obtener el tamaño de un mensaje antes de enviarlo.
- strncmp(): comparar mensajes (ej: identificar "SUBSCRIBER" o "PUBLISHER").

--------------------------------------------------

## <stdlib.h>
Se utiliza para funciones generales del sistema, incluyendo generación de números aleatorios.

Funciones usadas:
- rand(): genera números aleatorios.

Uso en el sistema:
Se utiliza para asignar un puerto dinámico al subscriber:

Ejemplo:
miDireccion.sin_port = htons(6000 + rand()%1000);

Explicación:
- rand(): genera un número aleatorio
- %1000: limita el valor entre 0 y 999
- 6000 +: ajusta el rango entre 6000 y 6999
- htons(): convierte el puerto a formato de red

--------------------------------------------------

## <unistd.h>
Contiene funciones del sistema operativo.

Funciones usadas:
- read(): leer datos desde un socket TCP.
- close(): cerrar sockets y liberar recursos.

--------------------------------------------------

## <arpa/inet.h>
Librería fundamental para redes IPv4.

Elementos usados:
- struct sockaddr_in: estructura para representar direcciones IP + puerto.
- htons(): convierte el puerto a formato de red.
- inet_addr(): convierte una IP en texto a formato numérico.
- INADDR_ANY: permite usar cualquier dirección IP disponible en la máquina.

--------------------------------------------------

## <sys/socket.h>
Librería principal para la creación y manejo de sockets.

Funciones usadas:
- socket()
- bind()
- listen()
- accept()
- connect()
- send()
- sendto()
- recv()
- recvfrom()

Es la base de toda la comunicación del sistema.

--------------------------------------------------

## <sys/select.h>
Se usa únicamente en el broker TCP.

Permite manejar múltiples sockets simultáneamente.

Usamos:
- fd_set: Es una estructura que guarda el conjunto de sockets que se quieren monitorear (lo usamos en el broker TCP para vigilar el socket de escucha y los sockets de los clientes conectados).
- FD_ZERO(): Se usa en el ciclo infinito del broker TCP para limpiar el conjunto de sockets vigilados antes de volver a agregar el socket de escucha y los sockets de los clientes activos.
- FD_SET(): Se usa en el broker TCP para agregar al conjunto vigilado primero el socket de escucha y luego los sockets de cada publisher o subscriber conectado.
- FD_ISSET(): Se usa después del select() en el broker TCP para verificar si hubo actividad en el socket de escucha (nueva conexión) o en alguno de los sockets de los clientes (mensaje o desconexión).
- select(): Se usa en el broker TCP para monitorear varios sockets simultáneamente y quedarse bloqueado hasta que alguno tenga actividad (que puede ser o nueva conexión o llegada de mensaje).

--------------------------------------------------

## ¿Qué es sockaddr y sockaddr_in?

- struct sockaddr: estructura genérica de direcciones de socket.
- struct sockaddr_in: versión específica para IPv4.

Se usa casting (struct sockaddr*) porque las funciones de sockets trabajan con la estructura genérica y permiten soportar distintos tipos de direcciones (IPv4, IPv6, etc.).

Campos importantes:
- sin_family->tipo de dirección (AF_INET)
- sin_addr.s_addr->dirección IP
- sin_port->puerto (convertido con htons)

--------------------------------------------------

## socket()

Crea un socket de comunicación.

Ejemplo:
socket(AF_INET, SOCK_STREAM, 0);

Parámetros:
- AF_INET: IPv4
- SOCK_STREAM: TCP (conexión)
- SOCK_DGRAM: UDP (sin conexión)
- 0: protocolo automático

Retorna:
- Un descriptor de socket
- -1 en caso de error

--------------------------------------------------

## bind()

Asocia un socket a una dirección IP y puerto.

Ejemplo:
bind(socket, (struct sockaddr*)&direccion, sizeof(direccion));

Parámetros:
- socket: descriptor
- dirección: estructura sockaddr
- tamaño: tamaño de la estructura

--------------------------------------------------

## listen()

Activa modo escucha (TCP).

Ejemplo:
listen(socketEscucha, 5);

Parámetros:
- socketEscucha: socket previamente enlazado
- 5: número máximo de conexiones en espera (backlog)

--------------------------------------------------

## accept()

Acepta conexiones entrantes.

Ejemplo:
accept(socketEscucha, (struct sockaddr*)&dir, &tam);

Parámetros:
- socketEscucha
- dirección del cliente
- tamaño de la dirección

Retorna un nuevo socket para el cliente.

--------------------------------------------------

## connect()

Conecta cliente a servidor.

Ejemplo:
connect(socket, (struct sockaddr*)&dir, sizeof(dir));

Parámetros:
- socket
- dirección servidor
- tamaño

--------------------------------------------------

## send()

Envía datos en TCP.

Ejemplo:
send(socket, buffer, strlen(buffer), 0);

Parámetros:
- socket
- buffer
- tamaño
- flags (0)

--------------------------------------------------

## sendto()

Envía datos en UDP.

Ejemplo:
sendto(socket, buffer, len, 0, (struct sockaddr*)&dir, sizeof(dir));

Parámetros:
- socket
- buffer
- tamaño
- flags
- dirección destino
- tamaño dirección

--------------------------------------------------

## read()

Proviene de <unistd.h>.
Se usa para leer datos desde sockets TCP.

--------------------------------------------------

## recv()

Proviene de <sys/socket.h>.
Se usa para recibir datos en sockets.

--------------------------------------------------

## recvfrom()

Recibe datos en UDP.

Ejemplo:
recvfrom(socket, buffer, max, 0, (struct sockaddr*)&dir, &tam);

Parámetros:
- socket
- buffer
- tamaño
- flags
- dirección origen
- tamaño dirección

--------------------------------------------------

## select()

Permite manejar múltiples conexiones sin usar hilos.

Ejemplo:
select(max+1, &set, NULL, NULL, NULL);

Parámetros:
- max+1: mayor descriptor + 1
- set: sockets vigilados
- NULL: escritura
- NULL: excepciones
- NULL: espera indefinida

--------------------------------------------------

## close()

Cierra un socket.

Ejemplo:
close(socket);

Libera recursos del sistema.

--------------------------------------------------

## Conclusión

Se documentó completamente:
- Librerías utilizadas
- Funciones de sockets
- Parámetros
- Estructuras de red

El sistema implementa correctamente:

Publisher->Broker->Subscriber

- TCP: comunicación confiable
- UDP: comunicación rápida sin conexión


--------------------------------------------------

# Uso de funciones en el sistema

## socket()
- broker_tcp.c->crear el socket servidor TCP que escucha conexiones de publishers y subscribers.
- broker_udp.c->crear el socket servidor UDP para recibir mensajes sin conexión.
- publisher_tcp.c->crear socket cliente TCP para conectarse al broker.
- publisher_udp.c->crear socket cliente UDP para enviar eventos al broker.
- subscriber_tcp.c->crear socket cliente TCP para conectarse al broker.
- subscriber_udp.c->crear socket UDP para registrarse y recibir eventos.

--------------------------------------------------

## bind()
- broker_tcp.c->asociar el socket TCP del broker al puerto 5000.
- broker_udp.c->asociar el socket UDP del broker al puerto 5000.
- subscriber_udp.c->asociar el socket UDP a un puerto dinámico generado con rand().

--------------------------------------------------

## listen()
- broker_tcp.c->poner el socket TCP en estado de escucha.

--------------------------------------------------

## accept()
- broker_tcp.c->aceptar conexiones TCP de clientes y crear sockets individuales.

--------------------------------------------------

## connect()
- publisher_tcp.c->establecer conexión TCP con el broker.
- subscriber_tcp.c->conectarse al broker para registrarse.

--------------------------------------------------

## send()
- publisher_tcp.c->enviar eventos al broker.
- broker_tcp.c->reenviar eventos a los subscribers.

--------------------------------------------------

## recv()
- broker_tcp.c->recibir eventos de publishers.
- subscriber_tcp.c->recibir eventos del broker.

--------------------------------------------------

## sendto()
- publisher_udp.c->enviar eventos sin conexión al broker.
- broker_udp.c->reenviar eventos a subscribers UDP.

--------------------------------------------------

## recvfrom()
- broker_udp.c->recibir mensajes de publishers UDP.
- subscriber_udp.c->recibir eventos del broker.

--------------------------------------------------

## select()
- broker_tcp.c->monitorear múltiples sockets simultáneamente.

--------------------------------------------------

## close()
- todos los archivos->cerrar sockets al finalizar o desconectarse.

--------------------------------------------------

## rand()
- subscriber_udp.c->generar un puerto dinámico entre 6000 y 6999.

