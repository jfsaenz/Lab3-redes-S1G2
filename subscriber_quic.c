#include <stdio.h>   
#include <stdlib.h>     
#include <string.h> 
#include <unistd.h>  
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

int main() {

    int puertoBroker = 5000; //Ponemos el puerto del broker al que nos vamos a conectar

    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el subscriber puede leer en una sola lectura (en un fragmento) del mensaje que le mande el broker (o sea del evento de futbol)

    int maxReintentos = 5; //Es el número máximo de veces que el subscriber reintenta registrarse si no recibe ACK del broker

    int socketSubscriber; //Es el socket que el subscriber utiliza para recibir los mensajes de eventos de futbol que le mande el broker

    struct sockaddr_in direccionSocketBroker; //Es una estructura que nos ayuda a representar la dirección del socket del broker (dirección IP de la máquina donde se ejecuta el broker + el puerto)

    struct sockaddr_in miDireccion; //Es una estructura que nos ayuda a representar la dirección del socket del subscriber (dirección IP de la máquina donde se ejecuta el subscriber + el puerto dinámico)

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el subscriber lee del broker (o sea los eventos de futbol que le manda el broker)

    //Último número de secuencia recibido del broker
    //Lo usamos para verificar que los mensajes lleguen en orden (característica QUIC de orden)
    //Si llega un mensaje con número de secuencia menor o igual al último que procesamos, sabemos que es duplicado
    //Si llega uno mayor al esperado, sabemos que se perdió un mensaje intermedio (hueco)
    int ultimoSeqRecibido = 0;

    //Semilla para rand() usando el tiempo actual + PID del proceso
    //para que cada subscriber que se abra tenga un puerto diferente
    srand(time(NULL) + getpid());

    //Creamos el socket y le pasamos que usara direcciones IPv4, que es un socket UDP y el protocolo UDP
    socketSubscriber = socket(AF_INET, SOCK_DGRAM, 0);

    miDireccion.sin_family = AF_INET; //Le decimos a la dirección del socket del subscriber que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto dinámico del subscriber
    miDireccion.sin_addr.s_addr = INADDR_ANY; //Le decimos que el subscriber se va a conectar al broker usando cualquier IP disponible, porque el subscriber no necesita una IP específica
    miDireccion.sin_port = htons(7000 + rand()%1000); //Le decimos que el subscriber se va a conectar al broker usando un puerto dinámico (o sea uno random)
    //OJITO: Aqui nuestro puerto dinamico puede ser entre 7000 y 7999 (usamos un rango diferente al subscriber_udp que usa 6000-6999 para no interferir)

    //Hacemos bind para asignarle al socket del subscriber la dirección del socket del subscriber (IP y puerto dinámico) para que el subscriber pueda recibir mensajes en esa dirección
    bind(socketSubscriber, (struct sockaddr*)&miDireccion, sizeof(miDireccion));

    direccionSocketBroker.sin_family = AF_INET; //Le decimos a la dirección del socket del broker que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto del broker
    direccionSocketBroker.sin_addr.s_addr = inet_addr("127.0.0.1"); //Le decimos que el subscriber se va a conectar al broker usando la IP localhost, porque ambos se ejecutan en la misma máquina virtual.
    direccionSocketBroker.sin_port = htons(puertoBroker); //Le decimos a la dirección del socket del broker el puerto donde el broker escucha las conexiones UDP (que es el puerto 5000)

    //Ahora tenemos que enviar un mensaje al broker para identificarnos como subscriber
    //Pero a diferencia del subscriber UDP normal, aquí esperamos ACK del broker para confirmar que nos registró
    //Si no llega el ACK retransmitimos (características QUIC de ACKs y retransmisión)
    int registrado = 0;
    for(int intento=0; intento<maxReintentos && !registrado; intento++){

        //Enviamos "SUBSCRIBER" al broker para que nos registre
        sendto(socketSubscriber, "SUBSCRIBER", strlen("SUBSCRIBER"), 0, (struct sockaddr *)&direccionSocketBroker, sizeof(direccionSocketBroker));

        if(intento > 0){
            printf("Reintentando registro (intento %d)\n", intento + 1);
        }

        //Esperamos ACK de registro del broker con timeout de 1 segundo usando select()
        fd_set conjuntoLectura;
        struct timeval timeout;
        FD_ZERO(&conjuntoLectura);
        FD_SET(socketSubscriber, &conjuntoLectura);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int actividad = select(socketSubscriber + 1, &conjuntoLectura, NULL, NULL, &timeout);

        if(actividad > 0 && FD_ISSET(socketSubscriber, &conjuntoLectura)){
            struct sockaddr_in dirRespuesta;
            socklen_t tamRespuesta = sizeof(dirRespuesta);
            int bytesRecibidos = recvfrom(socketSubscriber, bufferMensaje, maxBytesFragmento, 0, (struct sockaddr*)&dirRespuesta, &tamRespuesta);
            if(bytesRecibidos > 0){
                bufferMensaje[bytesRecibidos] = '\0';
                //Si el broker nos mandó "ACK:REGISTRO" es porque nos registró exitosamente
                if(strncmp(bufferMensaje, "ACK:REGISTRO", 12)==0){
                    registrado = 1;
                }
            }
        }
        //Si no llegó nada en 1 segundo (timeout), el for vuelve a intentar enviando "SUBSCRIBER" de nuevo
    }

    //Si todo salió bien, esto debería aparecer
    printf("Subscriber QUIC listo\n");

    //Ahora si podemos recibir los eventos entonces:
    //Hacemos un ciclo infinito para que el subscriber siempre esté esperando a recibir mensajes del broker
    while(1) {

        //Leemos el mensaje que el socket del subscriber recibe del broker y lo guardamos en el buffer de mensaje
        //también guardamos la cantidad de bytes que se leen del mensaje que manda el broker (o sea del evento de futbol)
        int cantidadBytesLeidos = recvfrom(socketSubscriber, bufferMensaje, maxBytesFragmento, 0, NULL, NULL);

        //Si la cantidad de bytes es menor o igual a 0, es porque el broker se desconectó
        //En ese caso, salimos del ciclo infinito y cerraríamos el socket del subscriber porque ya no tendría sentido seguir esperando mensajes 
        if(cantidadBytesLeidos <= 0){    
            printf("Broker desconectado\n");
            break;
        }

        //Le ponemos el caracter de fin para poder imprimirlo
        bufferMensaje[cantidadBytesLeidos] = '\0';

        //Los mensajes del broker QUIC vienen con formato "SEQ:XXXX|mensaje"
        //Tenemos que extraer el número de secuencia y el contenido del mensaje
        if(strncmp(bufferMensaje, "SEQ:", 4)==0){

            //Extraemos el número de secuencia del mensaje
            int seqRecibido = 0;
            sscanf(bufferMensaje + 4, "%d", &seqRecibido);

            //Buscamos el contenido del mensaje (lo que viene después del "|")
            char *contenido = strchr(bufferMensaje, '|');

            if(contenido != NULL){
                contenido++; //Saltamos el caracter "|"

                //Verificamos el orden del mensaje usando el número de secuencia (característica QUIC de orden)
                if(seqRecibido == ultimoSeqRecibido + 1){
                    //El mensaje llegó en el orden correcto (es el siguiente que esperábamos)
                    printf("Evento recibido (SEQ:%04d): %s\n", seqRecibido, contenido);
                    ultimoSeqRecibido = seqRecibido;
                }
                else if(seqRecibido > ultimoSeqRecibido + 1){
                    //Llegó un mensaje pero faltan mensajes intermedios (hueco en la secuencia)
                    //Esto indica que se perdió algún paquete en el camino
                    printf("Evento recibido con hueco (SEQ:%04d, esperaba %04d): %s\n", seqRecibido, ultimoSeqRecibido + 1, contenido);
                    ultimoSeqRecibido = seqRecibido;
                }
                else{
                    //Es un duplicado o llegó tarde (ya procesamos este número de secuencia o uno mayor)
                    //Esto puede pasar si el broker retransmitió un mensaje que ya habíamos recibido
                    printf("Evento duplicado (SEQ:%04d, ultimo procesado: %04d): %s\n", seqRecibido, ultimoSeqRecibido, contenido);
                }

                //Enviamos ACK al broker para confirmar que recibimos este evento (característica QUIC de ACKs)
                //Formato del ACK: "ACK:XXXX" con el número de secuencia del mensaje que recibimos
                char ackEvento[maxBytesFragmento];
                snprintf(ackEvento, maxBytesFragmento, "ACK:%04d", seqRecibido);
                sendto(socketSubscriber, ackEvento, strlen(ackEvento), 0, (struct sockaddr *)&direccionSocketBroker, sizeof(direccionSocketBroker));
            }
        }
        else{
            //Si el mensaje no tiene formato SEQ, lo imprimimos tal cual
            printf("Evento recibido: %s\n", bufferMensaje);
        }
    }

    close(socketSubscriber);

    return 0;
}