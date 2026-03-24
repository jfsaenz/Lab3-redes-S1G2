#include <stdio.h>     
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>  
#include <arpa/inet.h> 

int main() {

    int puertoBroker = 5000; //Ponemos el puerto del broker al que nos vamos a conectar

    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el publisher puede escribir en una sola escritura (en un fragmento) del mensaje que le mande el broker (o sea del evento de futbol)

    int socketPublisher; //Es el socket que el publisher utiliza para enviar los mensajes de eventos de futbol al broker a través de UDP

    struct sockaddr_in direccionBroker; //Es una estructura que nos ayuda a representar la dirección del socket del broker (dirección IP de la máquina donde se ejecuta el broker + el puerto donde el broker recibe los mensajes UDP)

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el publisher escribe para enviarlos al broker (o sea los eventos de futbol)

    //Creamos el socket UDP del publisher

    //Le decimos que el socket UDP del publisher use direcciones IPv4, que es un socket UDP y el protocolo UDP
    socketPublisher = socket(AF_INET, SOCK_DGRAM, 0);

    direccionBroker.sin_family = AF_INET; //Le decimos a la dirección del socket del broker que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto del broker
    direccionBroker.sin_addr.s_addr = inet_addr("127.0.0.1"); //Le decimos que el publisher se va a conectar al broker usando la IP localhost, porque ambos se ejecutan en la misma máquina virtual (broker y publisher)
    direccionBroker.sin_port = htons(puertoBroker); //Le decimos a la dirección del socket del broker el puerto donde el broker recibe los mensajes UDP (que es el puerto 5000)

    //OJITO: No hacemos connect porque UDP no es orientado a conexión
    //Solo enviamos mensajes (no confirmamos que el broker los reciba, pero si los recibe el broker los manda a los subscribers)

    //Si todo salió bien, esto debería aparecer
    printf("Publisher UDP listo\n");

    //Hacemos un ciclo infinito para que el publisher siempre esté esperando a escribir eventos de futbol para enviarlos al broker a través de UDP
    while(1) {

        //Pedimos al usuario que usa el publisher que escriba un evento de futbol para enviarlo al broker
        printf("Ingrese evento: ");

        //Ahora guardamos lo que el usuario escriba en el buffer de mensaje para luego enviarlo al broker a través de UDP
        fgets(bufferMensaje, maxBytesFragmento, stdin);

        //Le enviamos el mensaje que el usuario escribió al broker para que el broker se lo mande a los subscribers conectados a través de UDP
        sendto(socketPublisher, bufferMensaje, strlen(bufferMensaje), 0, (struct sockaddr *)&direccionBroker, sizeof(direccionBroker));
    }

    //Cerramos el socket en caso de que algo pase pero en realidad esto no deberia pasar nunca porque el ciclo es infinito
    //Solo por si acaso:
    close(socketPublisher);

    return 0;
}