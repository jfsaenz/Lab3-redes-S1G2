#include <stdio.h>     
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>  
#include <arpa/inet.h>
#include <sys/select.h>

int main() {

    int puertoBroker = 5000; //Ponemos el puerto del broker al que nos vamos a conectar

    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el publisher puede escribir en una sola escritura (en un fragmento) del mensaje que le mande al broker (o sea del evento de futbol)

    int maxReintentos = 5; //Es el número máximo de veces que el publisher reintenta enviar un mensaje al broker si no recibe ACK (característica QUIC de retransmisión)

    int socketPublisher; //Es el socket que el publisher utiliza para enviar los mensajes de eventos de futbol al broker a través de UDP

    struct sockaddr_in direccionBroker; //Es una estructura que nos ayuda a representar la dirección del socket del broker (dirección IP de la máquina donde se ejecuta el broker + el puerto donde el broker recibe los mensajes UDP)

    char bufferMensaje[maxBytesFragmento]; //Es el espacio en memoria donde se guardan los mensajes que el publisher escribe para enviarlos al broker (o sea los eventos de futbol)
    char bufferEnvio[maxBytesFragmento]; //Es el espacio en memoria donde armamos el mensaje con número de secuencia antes de enviarlo
    char bufferRecepcion[maxBytesFragmento]; //Es el espacio en memoria donde guardamos los ACKs que nos manda el broker

    //Número de secuencia del publisher: se incrementa con cada evento enviado
    //Cada evento que el publisher manda tiene un número de secuencia único y creciente para que el broker sepa el orden (característica QUIC de orden)
    int miNumSecuencia = 0;

    //Creamos el socket UDP del publisher
    //Le decimos que el socket UDP del publisher use direcciones IPv4, que es un socket UDP y el protocolo UDP
    socketPublisher = socket(AF_INET, SOCK_DGRAM, 0);

    direccionBroker.sin_family = AF_INET; //Le decimos a la dirección del socket del broker que tiene una dirección IPv4 para poderla configurar después con la dirección IP y el puerto del broker
    direccionBroker.sin_addr.s_addr = inet_addr("127.0.0.1"); //Le decimos que el publisher se va a conectar al broker usando la IP localhost, porque ambos se ejecutan en la misma máquina virtual (broker y publisher)
    direccionBroker.sin_port = htons(puertoBroker); //Le decimos a la dirección del socket del broker el puerto donde el broker recibe los mensajes UDP (que es el puerto 5000)

    //OJITO: No hacemos connect porque UDP no es orientado a conexión
    //Tampoco hacemos bind porque el publisher no necesita un puerto fijo, el sistema operativo le asigna uno automáticamente

    //Si todo salió bien, esto debería aparecer
    printf("Publisher QUIC listo\n");

    //Hacemos un ciclo infinito para que el publisher siempre esté esperando a escribir eventos de futbol para enviarlos al broker
    while(1) {

        //Pedimos al usuario que usa el publisher que escriba un evento de futbol para enviarlo al broker
        printf("Ingrese evento: ");

        //Ahora guardamos lo que el usuario escriba en el buffer de mensaje para luego enviarlo al broker a través de UDP
        fgets(bufferMensaje, maxBytesFragmento, stdin);

        //Quitamos el salto de línea del final que deja fgets
        bufferMensaje[strcspn(bufferMensaje, "\n")] = '\0';

        //Si el usuario no escribió nada, ignoramos y pedimos de nuevo
        if(strlen(bufferMensaje) == 0) continue;

        //Incrementamos nuestro número de secuencia como publisher (característica QUIC de orden)
        miNumSecuencia++;

        //Armamos el mensaje con formato QUIC: "SEQ:XXXX|mensaje"
        //Por ejemplo: "SEQ:0001|Gol de Equipo A al minuto 32"
        //Así el broker sabe el orden del mensaje
        snprintf(bufferEnvio, maxBytesFragmento, "SEQ:%04d|%s", miNumSecuencia, bufferMensaje);

        //Le enviamos el mensaje al broker pero a diferencia del publisher UDP normal, aquí esperamos ACK del broker
        //Si no llega el ACK en 500ms, retransmitimos hasta maxReintentos veces (característica QUIC de retransmisión y ACKs)
        int ackRecibido = 0; //Bandera para saber si el broker confirmó que recibió el evento

        for(int intento=0; intento<maxReintentos && !ackRecibido; intento++){

            //Enviamos el mensaje con número de secuencia al broker
            sendto(socketPublisher, bufferEnvio, strlen(bufferEnvio), 0, (struct sockaddr *)&direccionBroker, sizeof(direccionBroker));

            if(intento > 0){
                printf("Retransmision SEQ:%04d (intento %d)\n", miNumSecuencia, intento + 1);
            }

            //Esperamos ACK del broker con timeout usando select()
            //select() nos permite esperar un tiempo limitado (500ms) a que llegue algo al socket
            fd_set conjuntoACK;
            struct timeval timeoutACK;

            FD_ZERO(&conjuntoACK); //Limpiamos el conjunto de sockets vigilados
            FD_SET(socketPublisher, &conjuntoACK); //Agregamos el socket del publisher al conjunto

            timeoutACK.tv_sec = 0; //0 segundos
            timeoutACK.tv_usec = 500000; //500000 microsegundos = 500ms de timeout

            //select se queda esperando hasta que llegue algo al socket o hasta que se cumpla el timeout
            int actividad = select(socketPublisher + 1, &conjuntoACK, NULL, NULL, &timeoutACK);

            if(actividad > 0 && FD_ISSET(socketPublisher, &conjuntoACK)){
                //Llegó algo al socket, lo leemos para ver si es el ACK que esperamos
                struct sockaddr_in dirACK;
                socklen_t tamACK = sizeof(dirACK);
                int bytesACK = recvfrom(socketPublisher, bufferRecepcion, maxBytesFragmento, 0, (struct sockaddr*)&dirACK, &tamACK);

                if(bytesACK > 0){
                    bufferRecepcion[bytesACK] = '\0';

                    //Verificamos que sea el ACK correcto para nuestro número de secuencia
                    //El ACK del broker tiene formato: "ACK:XXXX" donde XXXX es el número de secuencia
                    char ackEsperado[maxBytesFragmento];
                    snprintf(ackEsperado, maxBytesFragmento, "ACK:%04d", miNumSecuencia);

                    if(strncmp(bufferRecepcion, ackEsperado, strlen(ackEsperado)) == 0){
                        ackRecibido = 1;
                        printf("Broker confirmo SEQ:%04d\n", miNumSecuencia);
                    }
                    //Si el ACK no coincide con el esperado, seguimos intentando
                }
            }
            //Si no llegó nada en 500ms (timeout), el for vuelve a intentar enviando el mensaje de nuevo
        }
    }

    //Cerramos el socket en caso de que algo pase pero en realidad esto no deberia pasar nunca porque el ciclo es infinito
    //Solo por si acaso:
    close(socketPublisher);

    return 0;
}