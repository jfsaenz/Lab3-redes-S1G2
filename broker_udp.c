#include <stdio.h>    
#include <stdlib.h>   
#include <string.h>  
#include <unistd.h>     
#include <arpa/inet.h>  

int main(){

    //Definimos ciertas variables importantes para que nuestro broker funcione
    int puerto = 5000; //Es el puerto donde el broker queda a la espera de solicitudes de conexión de publishers y subscribers, a traves del socket UDP 

    //El socket udp es diferente al socket tcp porque como el udp no es orientado a conexión
    //no hay un socket de escucha ni sockets de clientes conectados
    //sino que el broker simplemente tiene un socket udp para recibir mensajes de los publishers y subscribers y para enviar mensajes a los subscribers
    
    int maxClientes = 50; //Es el número máximo de subscribers que el broker puede manejar a la vez, o sea el número máximo de subscribers a los que el broker les puede enviar eventos de futbol a través de UDP
   
    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el broker puede leer en una sola lectura (en un fragmento) del mensaje que le mande el publisher a través de UDP (o sea del evento de futbol)

    int socketUDPBroker; //Es el socket que el broker utiliza para recibir mensajes de los publishers y subscribers a través de UDP y para enviar mensajes a los subscribers a través de UDP


    //Arreglo de sockets de los subscribers conectados al broker:
    //Hacemos un arreglo de los subscribers que van a recibir los eventos de futbol que les mande el broker a través del socket UDP
    //Este arreglo es necesario porque el broker tiene que saber a qué subscribers enviarles los eventos de futbol que le manden los publishers
    struct sockaddr_in listaSubscribers[maxClientes]; //Cada elemento es una estructura que representa la dirección de un subscriber (dirección IP del subscriber + puerto del subscriber, es decir, el socket de cada subscriber)

    int totalSubscribers = 0; //Llevamos el número de subscribers que están registrados en el broker (el numero de elementos en el arreglo de subscribers)

    //Ahora hacemos una estructura para la dirección del socket udp del broker (direccion ip + puerto)
    struct sockaddr_in direccionSocketUDPBroker;

    //Hacemos una estructura para la dirección del socket de cada cliente (subscriber o publisher) que llegue al broker a través de UDP
    struct sockaddr_in direccionSocketCliente;

    //Guardamos el tamaño de la estructura de la dirección del socket del cliente
    socklen_t tamanioDireccionSocketCliente = sizeof(direccionSocketCliente);

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el broker lee de los publishers a través de UDP (o sea los eventos de futbol que le mandan los publishers)

    //Creamos el socket UDP del broker
    //Le decimos que el socket UDP del broker use direcciones IPv4, que es un socket UDP y el protocolo UDP
    socketUDPBroker = socket(AF_INET, SOCK_DGRAM, 0);

    //Configuramos la dirección del socket UDP del broker 
    direccionSocketUDPBroker.sin_family = AF_INET; //Le decimos que el socket UDP del broker usará direcciones IPv4 para poderlo terminar de configurar
    direccionSocketUDPBroker.sin_addr.s_addr = INADDR_ANY; //El socket UDP del broker acepta mensajes por cualquier dirección IP que tenga la máquinaa en la que se ejecuta el broker 
    direccionSocketUDPBroker.sin_port = htons(puerto); //Le digo al socket UDP del broker el puerto donde va a recibir los mensajes UDP de los publishers y subscribers (que es el puerto 5000)

    //Hasta acá el socket UDP del broker existe y sabe que será UDP pero ahora si le vamos a asignar la dirección IP + el puerto para que quede listo para recibir mensajes UDP de los publishers y subscribers
    bind(socketUDPBroker, (struct sockaddr*)&direccionSocketUDPBroker, sizeof(direccionSocketUDPBroker));

    //Si todo salió bien, esto debería aparecer indicando que el broker UDP está listo para recibir mensajes de los publishers y subscribers a través de UDP
    printf("Broker UDP listo\n");

    //Acá no hacemos listen porque no estamos esperando escuchar solicitudes de conexion, simplemente vamos de una con recibir mensajes UDP

    //Ahora hacemos un ciclo infinito para que el broker siempre esté esperando a recibir mensajes de los publishers y subscribers a través de UDP
    //y también para que siempre esté enviando los eventos de futbol a los subscribers registrados a través de UDP

    while(1){
        //El broker recibirá mensajes de los clientes (publisers o subscribers) a través de UDP
        //Aqui obtenemos el número de bytes que vienen en el mensaje que le llegó al broker
        //También guardamos el mensaje que llegó al broker en el buffer
        //Guardamos la dirección del socket del cliente que mandó el mensaje
        int bytesRecibidos = recvfrom(socketUDPBroker, bufferMensaje, maxBytesFragmento, 0, (struct sockaddr*)&direccionSocketCliente, &tamanioDireccionSocketCliente);

        //Le ponemos el caracter de fin al mensaje para poderlo imprimir bien después
        bufferMensaje[bytesRecibidos] = '\0';

        //Si el mensaje que le llegó al broker empieza con "SUBSCRIBER", es porque el cliente que mandó ese mensaje es un subscriber que se está registrando para recibir los eventos de futbol a través de UDP
        if(strncmp(bufferMensaje,"SUBSCRIBER",10)==0){
            //Guardamos la dirección del socket del subscriber que se está registrando en el arreglo de subscribers, así luego le podemos enviar los eventos de futbol
            listaSubscribers[totalSubscribers] = direccionSocketCliente;
            //Aumentamos el número de subscribers registrados
            totalSubscribers++;
            //Aqui si todo salio bien ponemos que si se agregó el subscriber
            printf("Subscriber agregado\n");
            //Revisamos el siguiente mensaje
            continue;
        }

        //Si el mensaje que le llegó al broker no es "SUBSCRIBER", entonces es un mensaje de evento de futbol que le mandó un publisher a través de UDP
        
        //Imprimimos el evento de futbol que llegó al broker
        printf("Evento: %s\n",bufferMensaje);

        //Reenviamos el evento de futbol a todos los subscribers registrados a través de UDP
        for(int i=0; i<totalSubscribers; i++){
            //Enviamos a cada subscriber usando la dirección de su socket que guardamos en el arreglo de subscribers
            sendto(socketUDPBroker, bufferMensaje, strlen(bufferMensaje), 0, (struct sockaddr*)&listaSubscribers[i], sizeof(listaSubscribers[i]));
        }
    }
}