#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>    
#include <arpa/inet.h> 

int main() {

    int puertoBroker = 5000; //Ponemos el puerto del broker al que nos vamos a conectar

    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el subscriber puede leer en una sola lectura (en un fragmento) del mensaje que le mande el broker (o sea del evento de futbol)

    int socketSubscriber; //Es el socket que el subscriber utiliza para conectarse al broker y para recibir los mensajes de eventos de futbol que le mande el broker

    struct sockaddr_in direccionSocketBroker; //Es una estructura que nos ayuda a representar la dirección del socket del broker (dirección IP de la máquina donde se ejecuta el broker + el puerto donde el broker escucha las conexiones TCP)

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el subscriber lee del broker (o sea los eventos de futbol que le manda el broker)
   
    //Ahora creamos el socket del subscriber

    //Creamos el socket y le pasamos que usara direcciones IPv4, que es un socket TCP y el protocolo TCP
    socketSubscriber = socket(AF_INET, SOCK_STREAM, 0);

    direccionSocketBroker.sin_family = AF_INET; //Le decimos a la dirección del socket del broker que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto del broker

    direccionSocketBroker.sin_addr.s_addr = inet_addr("127.0.0.1"); //Le decimos que el subscriber se va a conectar al broker usando la IP localhost, porque ambos se ejecutan en la misma máquina virtual.
    //Básicamente como el broker y el subscriber se ejecutan en la misma máquina virtual,
    //usamos la dirección IP 127.0.0.1 (localhost, la de la VM misma) para conectarnos al broker
    //Esto significa que la conexión TCP se hace dentro de la misma máquina, básicamente el subcriber si sabe que el broker está en la misma máquina y se conecta a través de localhost
    
    direccionSocketBroker.sin_port = htons(puertoBroker); //Le decimos a la dirección del socket del broker el puerto donde el broker escucha las conexiones TCP (que es el puerto 5000)


    //Nos conectamos desde el socket del subscriber al socket del broker con la dirección del socket del broker (IP  puerto) para establecer la conexión TCP entre el subscriber y el broker
    
    //Hacemos connect y le pasamos el socket del subscriber, la dirección del socket del brokery el tamaño de esa estructura para conectarnos al broker a través de TCP
    connect(socketSubscriber, (struct sockaddr *)&direccionSocketBroker, sizeof(direccionSocketBroker));

    //Si todo salió bien, esto debería aparecer
    printf("Conectado al broker\n");

    //Nos identificamos como subscriber al enviarle un mensaje al broker que diga "SUBSCRIBER" para que sepa que somos un cliente subscriber y no un publisher
    //Cuando el broker sepa esto ya nos podrá enviar los eventos de futbol que le manden los publishers
    
    //Con el socket del subscriber, enviamos "SUBSCRIBER" literalmente
    send(socketSubscriber, "SUBSCRIBER", strlen("SUBSCRIBER"), 0);

    //Si todo salió bien, esto debería aparecer
    printf("Registrado como subscriber\n");

    //Hacemos un ciclo infinito para que el subscriber siempre esté esperando a recibir mensajes del broker
    while(1) {
        //Leemos el mensaje que el socket del subscriber recibe del broker y lo guardamos en el buffer de mensaje, también guardamos la cantidad de bytes que se leen del mensaje que manda el broker (o sea del evento de futbol)
        int cantidadBytesLeidos = read(socketSubscriber, bufferMensaje, maxBytesFragmento);

        //Si la cantidad de bytes es menor o igual a 0, es porque el broker se desconectó
        //En ese caso, salimos del ciclo infinito y cerraríamos el socket del subscriber porque ya no tendría sentido seguir esperando mensajes 
        if (cantidadBytesLeidos <= 0) {    
            printf("Broker desconectado\n");
            break;
        }

        //Le ponemos el caracter de fin al mensaje para poderlo imprimir bien
        bufferMensaje[cantidadBytesLeidos] = '\0';

        //Imprimimos el mensaje para verificar que todo esta bien y ver el evento de futbol que nos mandó el broker
        printf("Evento recibido: %s\n", bufferMensaje);
    }

    //Cerramos el socket porque el broker se desconectó 
    close(socketSubscriber);

    return 0;
}