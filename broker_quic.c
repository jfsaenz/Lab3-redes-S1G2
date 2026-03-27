#include <stdio.h>    
#include <stdlib.h>   
#include <string.h>  
#include <unistd.h>     
#include <arpa/inet.h>  
#include <sys/select.h>

int main(){

    //Definimos ciertas variables importantes para que nuestro broker QUIC funcione
    //Este broker es similar al broker UDP pero le agrega características de QUIC (que combina lo mejor de TCP y UDP):
    //1. ACKs: cada mensaje recibido genera una confirmación al remitente
    //2. Retransmisión: si un subscriber no confirma, el broker reenvía el mensaje
    //3. Orden: los mensajes llevan número de secuencia para que los subscribers sepan en qué orden llegaron
    //Todo esto se hace sobre UDP, o sea que seguimos usando sockets UDP pero con estas mejoras de confiabilidad
    int puerto = 5000; //Es el puerto donde el broker queda a la espera de mensajes de publishers y subscribers a través del socket UDP

    int maxClientes = 50; //Es el número máximo de subscribers que el broker puede manejar a la vez

    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el broker puede leer en una sola lectura

    int maxReintentos = 5; //Es el número máximo de veces que el broker reintenta enviar un mensaje a un subscriber si no recibe ACK

    int socketUDPBroker; //Es el socket que el broker utiliza para recibir mensajes de los publishers y subscribers a través de UDP y para enviar mensajes a los subscribers

    //Arreglo de subscribers registrados en el broker:
    //Hacemos un arreglo de los subscribers que van a recibir los eventos de futbol que les mande el broker
    //Este arreglo es necesario porque el broker tiene que saber a qué subscribers enviarles los eventos
    struct sockaddr_in listaSubscribers[maxClientes]; //Cada elemento es una estructura que representa la dirección de un subscriber (dirección IP + puerto)

    int totalSubscribers = 0; //Llevamos el número de subscribers que están registrados en el broker

    //Número de secuencia global del broker: cada evento que reenvía a los subscribers lleva un número de secuencia incremental
    //Así los subscribers pueden saber el orden correcto de los mensajes (característica QUIC de orden)
    int numSecuenciaGlobal = 0;

    //Ahora hacemos una estructura para la dirección del socket UDP del broker (dirección IP + puerto)
    struct sockaddr_in direccionSocketUDPBroker;

    //Hacemos una estructura para la dirección del socket de cada cliente (subscriber o publisher) que llegue al broker
    struct sockaddr_in direccionSocketCliente;

    //Guardamos el tamaño de la estructura de la dirección del socket del cliente
    socklen_t tamanioDireccionSocketCliente = sizeof(direccionSocketCliente);

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el broker lee de los publishers
    char bufferEnvio[maxBytesFragmento]; //Es el espacio en memoria donde el broker arma los mensajes con número de secuencia para enviarlos a los subscribers
    char bufferACK[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los ACKs que llegan de los subscribers

    //Creamos el socket UDP del broker
    //Le decimos que el socket UDP del broker use direcciones IPv4, que es un socket UDP y el protocolo UDP
    socketUDPBroker = socket(AF_INET, SOCK_DGRAM, 0);

    //Configuramos la dirección del socket UDP del broker
    direccionSocketUDPBroker.sin_family = AF_INET; //Le decimos que el socket UDP del broker usará direcciones IPv4 para poderlo terminar de configurar
    direccionSocketUDPBroker.sin_addr.s_addr = INADDR_ANY; //El socket UDP del broker acepta mensajes por cualquier dirección IP que tenga la máquina en la que se ejecuta el broker
    direccionSocketUDPBroker.sin_port = htons(puerto); //Le digo al socket UDP del broker el puerto donde va a recibir los mensajes UDP de los publishers y subscribers (que es el puerto 5000)

    //Hasta acá el socket UDP del broker existe y sabe que será UDP pero ahora si le vamos a asignar la dirección IP + el puerto para que quede listo
    bind(socketUDPBroker, (struct sockaddr*)&direccionSocketUDPBroker, sizeof(direccionSocketUDPBroker));

    //Si todo salió bien, esto debería aparecer indicando que el broker QUIC está listo
    printf("Broker QUIC listo\n");

    //Acá no hacemos listen porque estamos usando UDP, no TCP
    //Simplemente vamos de una con recibir mensajes UDP

    //Ahora hacemos un ciclo infinito para que el broker siempre esté esperando a recibir mensajes
    while(1){
        //El broker recibirá mensajes de los clientes (publishers o subscribers) a través de UDP
        //Aqui obtenemos el número de bytes que vienen en el mensaje que le llegó al broker
        //También guardamos el mensaje que llegó al broker en el buffer y la dirección del cliente que lo mandó
        int bytesRecibidos = recvfrom(socketUDPBroker, bufferMensaje, maxBytesFragmento, 0, (struct sockaddr*)&direccionSocketCliente, &tamanioDireccionSocketCliente);

        //Si no se recibió nada seguimos esperando
        if(bytesRecibidos <= 0) continue;

        //Le ponemos el caracter de fin al mensaje para poderlo imprimir bien después
        bufferMensaje[bytesRecibidos] = '\0';

        //Si el mensaje que le llegó al broker empieza con "SUBSCRIBER", es porque el cliente que mandó ese mensaje es un subscriber que se está registrando
        if(strncmp(bufferMensaje, "SUBSCRIBER", 10)==0){

            //Verificamos que no esté ya registrado para evitar duplicados
            int yaExiste = 0;
            for(int i=0; i<totalSubscribers; i++){
                if(listaSubscribers[i].sin_addr.s_addr == direccionSocketCliente.sin_addr.s_addr && listaSubscribers[i].sin_port == direccionSocketCliente.sin_port){
                    yaExiste = 1;
                    break;
                }
            }

            if(!yaExiste){
                //Guardamos la dirección del socket del subscriber que se está registrando en el arreglo de subscribers
                listaSubscribers[totalSubscribers] = direccionSocketCliente;
                //Aumentamos el número de subscribers registrados
                totalSubscribers++;
                printf("Subscriber agregado\n");
            }

            //Le enviamos un ACK de registro al subscriber para que sepa que fue registrado exitosamente
            //Esta es una característica QUIC: confirmamos que recibimos su mensaje de registro
            sendto(socketUDPBroker, "ACK:REGISTRO", strlen("ACK:REGISTRO"), 0, (struct sockaddr*)&direccionSocketCliente, sizeof(direccionSocketCliente));

            //Revisamos el siguiente mensaje
            continue;
        }

        //Si el mensaje empieza con "ACK:", es porque un subscriber está confirmando que recibió un evento
        //Solo lo registramos y seguimos, la lógica de retransmisión ya lo maneja más abajo
        if(strncmp(bufferMensaje, "ACK:", 4)==0){
            continue;
        }

        //Si no es ni SUBSCRIBER ni ACK, entonces es un mensaje de evento de futbol que le mandó un publisher

        //Primero: le enviamos ACK al publisher para confirmar que recibimos su evento (característica QUIC)
        //El publisher manda mensajes con formato "SEQ:XXXX|mensaje" así que extraemos el número de secuencia para el ACK
        char ackAlPublisher[maxBytesFragmento];
        if(strncmp(bufferMensaje, "SEQ:", 4)==0){
            //Extraemos los 4 dígitos del número de secuencia que viene después de "SEQ:"
            char seqStr[5];
            strncpy(seqStr, bufferMensaje + 4, 4);
            seqStr[4] = '\0';
            snprintf(ackAlPublisher, maxBytesFragmento, "ACK:%s", seqStr);
        } else {
            snprintf(ackAlPublisher, maxBytesFragmento, "ACK:0000");
        }

        //Enviamos el ACK al publisher para que sepa que el broker recibió su evento
        sendto(socketUDPBroker, ackAlPublisher, strlen(ackAlPublisher), 0, (struct sockaddr*)&direccionSocketCliente, sizeof(direccionSocketCliente));

        //Extraemos el contenido del mensaje (lo que viene después del "|")
        //Por ejemplo si el mensaje es "SEQ:0001|Gol de Equipo A al minuto 32", extraemos "Gol de Equipo A al minuto 32"
        char *contenido = strchr(bufferMensaje, '|');
        char mensajeEvento[maxBytesFragmento];
        if(contenido != NULL){
            contenido++; //Saltamos el caracter "|"
            strncpy(mensajeEvento, contenido, maxBytesFragmento - 1);
            mensajeEvento[maxBytesFragmento - 1] = '\0';
        } else {
            strncpy(mensajeEvento, bufferMensaje, maxBytesFragmento - 1);
            mensajeEvento[maxBytesFragmento - 1] = '\0';
        }

        //Imprimimos el evento de futbol que llegó al broker
        printf("Evento: %s\n", mensajeEvento);

        //Incrementamos el número de secuencia global del broker
        //Este número va en cada mensaje que el broker reenvía a los subscribers para que sepan el orden (característica QUIC de orden)
        numSecuenciaGlobal++;

        //Armamos el mensaje con el número de secuencia del broker para reenviarlo a los subscribers
        //Formato: "SEQ:XXXX|mensaje"
        snprintf(bufferEnvio, maxBytesFragmento, "SEQ:%04d|%s", numSecuenciaGlobal, mensajeEvento);

        //Reenviamos el evento de futbol a todos los subscribers registrados
        //Pero a diferencia del broker UDP normal, aquí esperamos ACK de cada subscriber
        //y si no llega el ACK retransmitimos (característica QUIC de retransmisión)
        for(int i=0; i<totalSubscribers; i++){

            int ackRecibido = 0; //Bandera para saber si el subscriber confirmó que recibió el evento

            //Intentamos enviar hasta maxReintentos veces si no recibimos ACK del subscriber
            for(int intento=0; intento<maxReintentos && !ackRecibido; intento++){

                //Enviamos el evento al subscriber usando la dirección de su socket que guardamos en el arreglo
                sendto(socketUDPBroker, bufferEnvio, strlen(bufferEnvio), 0, (struct sockaddr*)&listaSubscribers[i], sizeof(listaSubscribers[i]));

                if(intento > 0){
                    printf("Retransmision intento %d para subscriber %d (SEQ:%04d)\n", intento + 1, i + 1, numSecuenciaGlobal);
                }

                //Esperamos ACK del subscriber con timeout usando select()
                //select() nos permite esperar un tiempo limitado (500ms) a que llegue algo al socket
                //Si no llega nada en ese tiempo, asumimos que el subscriber no recibió el mensaje y retransmitimos
                fd_set conjuntoLectura;
                struct timeval timeout;

                FD_ZERO(&conjuntoLectura); //Limpiamos el conjunto de sockets vigilados
                FD_SET(socketUDPBroker, &conjuntoLectura); //Agregamos el socket del broker al conjunto

                timeout.tv_sec = 0; //0 segundos
                timeout.tv_usec = 500000; //500000 microsegundos = 500ms de timeout

                //select se queda esperando hasta que llegue algo al socket o hasta que se cumpla el timeout
                int actividad = select(socketUDPBroker + 1, &conjuntoLectura, NULL, NULL, &timeout);

                if(actividad > 0 && FD_ISSET(socketUDPBroker, &conjuntoLectura)){
                    //Llegó algo al socket, lo leemos
                    struct sockaddr_in dirRespuesta;
                    socklen_t tamRespuesta = sizeof(dirRespuesta);
                    int bytesACK = recvfrom(socketUDPBroker, bufferACK, maxBytesFragmento, 0, (struct sockaddr*)&dirRespuesta, &tamRespuesta);

                    if(bytesACK > 0){
                        bufferACK[bytesACK] = '\0';

                        //Verificamos que el ACK sea del subscriber correcto y del número de secuencia correcto
                        char ackEsperado[maxBytesFragmento];
                        snprintf(ackEsperado, maxBytesFragmento, "ACK:%04d", numSecuenciaGlobal);

                        if(strncmp(bufferACK, ackEsperado, strlen(ackEsperado))==0 && dirRespuesta.sin_addr.s_addr == listaSubscribers[i].sin_addr.s_addr && dirRespuesta.sin_port == listaSubscribers[i].sin_port){
                            ackRecibido = 1;
                        }

                        //Si llega un mensaje de registro de subscriber mientras esperamos ACK, lo registramos
                        if(strncmp(bufferACK, "SUBSCRIBER", 10)==0){
                            int yaExiste2 = 0;
                            for(int k=0; k<totalSubscribers; k++){
                                if(listaSubscribers[k].sin_addr.s_addr == dirRespuesta.sin_addr.s_addr && listaSubscribers[k].sin_port == dirRespuesta.sin_port){
                                    yaExiste2 = 1;
                                    break;
                                }
                            }
                            if(!yaExiste2){
                                listaSubscribers[totalSubscribers] = dirRespuesta;
                                totalSubscribers++;
                                printf("Subscriber agregado\n");
                            }
                            sendto(socketUDPBroker, "ACK:REGISTRO", strlen("ACK:REGISTRO"), 0, (struct sockaddr*)&dirRespuesta, sizeof(dirRespuesta));
                        }
                    }
                }
                //Si no llegó nada en 500ms (timeout), el for vuelve a intentar enviando el mensaje de nuevo
            }
        }
    }
}