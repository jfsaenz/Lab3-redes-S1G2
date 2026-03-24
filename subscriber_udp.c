#include <stdio.h>   
#include <stdlib.h>     
#include <string.h> 
#include <unistd.h>  
#include <arpa/inet.h> 

int main() {

    int puertoBroker = 5000; //Ponemos el puerto del broker al que nos vamos a conectar

    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el subscriber puede leer en una sola lectura (en un fragmento) del mensaje que le mande el broker (o sea del evento de futbol)

    int socketSubscriber; //Es el socket que el subscriber utiliza para conectarse al broker y para recibir los mensajes de eventos de futbol que le mande el broker

    struct sockaddr_in direccionSocketBroker; //Es una estructura que nos ayuda a representar la dirección del socket del broker (dirección IP de la máquina donde se ejecuta el broker + el puerto)

    struct sockaddr_in miDireccion; //Es una estructura que nos ayuda a representar la dirección del socket del subscriber (dirección IP de la máquina donde se ejecuta el subscriber + el puerto dinámico)

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el subscriber lee del broker (o sea los eventos de futbol que le manda el broker)

    //Para UDP, necesitamos una variable para el tamaño de la dirección del broker 
    //El tamaño hace referencia a la cantidad de bytes que ocupa la estructura de la dirección del socket del broker 
    socklen_t len = sizeof(direccionSocketBroker);

    //Creamos el socket y le pasamos que usara direcciones IPv4, que es un socket UDP y el protocolo UDP
    socketSubscriber = socket(AF_INET, SOCK_DGRAM, 0);

    miDireccion.sin_family = AF_INET; //Le decimos a la dirección del socket del subscriber que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto dinámico del subscriber
    miDireccion.sin_addr.s_addr = INADDR_ANY; //Le decimos que el subscriber se va a conectar al broker usando cualquier IP disponible, porque el subscriber no necesita una IP específica para conectarse al broker
    miDireccion.sin_port = htons(6000 + rand()%1000); //Le decimos que el subscriber se va a conectar al broker usando un puerto dinámico (o sea uno random)
    //OJITO: Aqui nuestro puerto dinamico puede ser entre 6000 y 6999 (por recomendación para que no haga interferencia con otros servicios que usan puertos menores a 6000)

    //Hacemos bind para asignarle al socket del subscriber la dirección del socket del subscriber (IP y puerto dinámico) para que el subscriber pueda recibir mensajes en esa dirección
    bind(socketSubscriber, (struct sockaddr*)&miDireccion, sizeof(miDireccion));

    
    direccionSocketBroker.sin_family = AF_INET; //Le decimos a la dirección del socket del broker que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto del broker
    direccionSocketBroker.sin_addr.s_addr = inet_addr("127.0.0.1"); //Le decimos que el subscriber se va a conectar al broker usando la IP localhost, porque ambos se ejecutan en la misma máquina virtual.
    direccionSocketBroker.sin_port = htons(puertoBroker); //Le decimos a la dirección del socket del broker el puerto donde el broker escucha las conexiones UDP (que es el puerto 5000)

    //Ahora tenemos que enviar un mensaje al broker para identificarnos como subscriber, porque el broker no sabe si el cliente que le está enviando mensajes es un publisher o un subscriber
    //Entonces al enviarle "SUBSCRIBER" el broker ya va a saber que es un subscriber y va a empezar a mandarle los eventos de futbol que le manden los publishers
    sendto(socketSubscriber, "SUBSCRIBER", strlen("SUBSCRIBER"), 0, (struct sockaddr *)&direccionSocketBroker, sizeof(direccionSocketBroker));

    //Si todo salió bien, esto debería aparecer
    printf("Subscriber UDP listo\n");

    //Ahora si podemos recibir los eventos entonces:
    //Hacemos un ciclo infinito para que el subscriber siempre esté esperando a recibir mensajes del broker
    while(1) {

        //Leemos el mensaje que el socket del subscriber recibe del broker y lo guardamos en el buffer de mensaje
        //también guardamos la cantidad de bytes que se leen del mensaje que manda el broker (o sea del evento de futbol)
        int cantidadBytesLeidos = recvfrom(socketSubscriber, bufferMensaje, maxBytesFragmento, 0, NULL, NULL);

        //Si la cantidad de bytes es menor o igual a 0, es porque el broker se desconectó
        //En ese caso, salimos del ciclo infinito y cerraríamos el socket del subscriber porque ya no tendría sentido seguir esperando mensajes 
        if (cantidadBytesLeidos <= 0) {    
            printf("Broker desconectado\n");
            break;
        }

        //Le ponemos el caracter de fin para poder imprimirloo
        bufferMensaje[cantidadBytesLeidos] = '\0';

        //Si todo salió bien, esto debería aparecer cada vez que el broker le mande un evento de futbol al subscriber
        printf("Evento recibido: %s\n", bufferMensaje);
    }

    close(socketSubscriber);

    return 0;
}