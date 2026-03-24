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

--------------------------------------------------

## <sys/select.h>
Se usa únicamente en el broker TCP.

Permite manejar múltiples sockets simultáneamente.

Elementos usados:
- fd_set
- FD_ZERO()
- FD_SET()
- FD_ISSET()
- select()

--------------------------------------------------

## ¿Qué es sockaddr y sockaddr_in?

- struct sockaddr: estructura genérica de direcciones de socket.
- struct sockaddr_in: versión específica para IPv4.

Se usa casting (struct sockaddr*) porque las funciones de sockets trabajan con la estructura genérica.

Campos importantes:
- sin_family → tipo de dirección (AF_INET)
- sin_addr.s_addr → dirección IP
- sin_port → puerto (convertido con htons)

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

## read() / recv()

Reciben datos en TCP.

Parámetros:
- socket
- buffer
- tamaño máximo

Retorna número de bytes leídos.

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

Publisher → Broker → Subscriber

- TCP: comunicación confiable
- UDP: comunicación rápida sin conexión
