#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

int main(){

    //Definimos ciertas variables importantes para que nuestro broker funcione
    int puerto = 5000; //Es el puerto donde el broker queda a la espera de solicitudes de conexión de publishers y subscribers, a traves del socket de escucha
    int maxClientes = 50; //Es el número máximo de conexiones TCP que el broker puede manejar a la vez
    int maxBytesFragmento = 1024; //Es la cantidad máxima de caracteres que el broker puede leer en una sola lectura (en un fragmento)
    //Aqui basta con 1024 porque los mensajes de goles, tarjetas, etc. no son muy largos

    int socketEscucha; //Es el socket que el broker utiliza para esperar y escuchar las peticiones de conexión TCP que llegan ya sea de publishers o subscribers (ej: conexion entre el broker y un publisher o entre el broker y un subscriber)
    int socketsClientes[maxClientes]; //Es un vector que guarda los sockets que son de las conexiones TCP activas entre el broker y los publishers o los subscribers
    int tipoCliente[maxClientes]; //Es un vector que nos ayuda a saber si el cliente que se conectó con el broker es un publisher (2) o un subscriber (1). 
    int totalClientesConectados = 0; //Para llevar la cuenta de cuántos clientes (publishers o subscribers) están conectados actualmente al broker con TCP

    struct sockaddr_in direccionSocketEscucha; //Es una estructura que nos ayuda a representar la dirección del socket de escucha (dirección IP de la máquina donde se ejecuta el broker + el puerto donde el broker escucha las conexiones TCP)
    int tamanioDireccionSocketEscucha = sizeof(direccionSocketEscucha); //Guarda el tamaño de la estructura direccionSocketEscucha, que usaremos luegoo

    fd_set socketsVigilados; //Es una estructura que sirve para que el broker vigile varios sockets al mismo tiempo (el socket de escucha y los sockets de los clientes conectados) y sepa cuándo hay actividad en alguno de ellos
    char bufferRecepcion[maxBytesFragmento]; //Espacio en memoria donde se guardan los mensajes que el broker lee de los publishers o subscribers

    //Creamos el socket que escucha, le pasamos que usara direcciones IPv4, que es un socket TCP y el protocolo TCP
    socketEscucha = socket(AF_INET, SOCK_STREAM, 0); //Esto tambien le da un ID al socket que se guarda en socketEscucha (o sea socketEscucha es un número que identifica al socket de escucha)


    direccionSocketEscucha.sin_family = AF_INET; //Le decimos que al socket usará direcciones IPv4 nuevamente para poderlo terminar de configurar
    direccionSocketEscucha.sin_addr.s_addr = INADDR_ANY; //El socket acepta conexiones por cualquier dirección IP que tenga esta máquina 
    //(ej: 127.0.0.1 que es conexión dentro de la misma máquina o 192.168.algo.algo que sería si hay otro PC que es cliente y quiere conectarse por wifi)
    direccionSocketEscucha.sin_port = htons(puerto); //Le digo al socket el puerto donde va a escuchar las conexiones TCP

    //Hasta acá el socket existe y sabe que será TCP pero ahora si le vamos a asignar la dirección IP + el puerto
    //Por eso mandamos el socket, la dirección del socket (que es la estructura direccionSocketEscucha) y el tamaño de esa estructura
    bind(socketEscucha, (struct sockaddr*)&direccionSocketEscucha, sizeof(direccionSocketEscucha));
    
    //Ponemos al socket a escuchar (esperar solicitudes de conexión TCP de publishers o subscribers con el broker) 
    //Por eso mandamos el socket y le decimos que puede haber hasta 5 conexiones TCP esperando a ser aceptadas por el broker (o sea hasta 5 publishers o subscribers esperando a que el broker los acepte para conectarse)
    //Si hay más de 5 conexiones TCP esperando, se rechazan de una
    listen(socketEscucha, 5);

    //Para saber que todo salió bien y el socket de escucha está listo para aceptar conexiones TCP
    //Eso significa que el broker ya está listo para aceptar conexiones TCP de publishers o subscribers 
    printf("Broker listo\n");

    //Hacemos un ciclo infinito para que el broker siempre esté esperando conexiones TCP de publishers o subscribers y también para que siempre esté vigilando los sockets de los clientes conectados (publishers o subscribers) para leer los mensajes que le manden
    while(1){

        //Como todo el tiempo puede haber clientes conectados y que se conecte uno o de desconecte otro, hay que actualizar constantemente el conjunto de sockets que el broker vigila 
        FD_ZERO(&socketsVigilados); //Por eso primero limpiamos el conjunto de sockets vigilados para eliminar los sockets que ya no están activos (porque se desconectaron)
        FD_SET(socketEscucha, &socketsVigilados); //Aqui agregamos el socket de escucha al conjunto de sockets vigilados para que el broker sepa cuando llegan solicitudes de conexión TCP 
        
        //Ahora tenemos que buscar el socket con el ID más alto para pasarlo al select
        //Esto es porque el select dice como voy a revisar los sockets vigilados, que van desde el ID 0 hasta el ID más alto que haya en el conjunto de sockets vigilados (o sea el socket de escucha o el socket de algún cliente conectado)
        //Sin saber el numero del socket con el ID más alto, el select no puede revisar correctamente los sockets vigilados porque no podria saber hasta qué número revisar 
        int mayorSocketVigilado = socketEscucha;

        //Por ahora el socket con el ID más alto es el socket de escucha, pero luego cuando haya clientes conectados, puede cambiar
        for(int i=0; i<totalClientesConectados; i++){
            //Agregamos al conjunto de sockets vigilados los sockets de los clientes conectados para que el broker sepa cuando llegan mensajes de los publishers o subscribers
            FD_SET(socketsClientes[i], &socketsVigilados); //Mandamos el socket del cliente conectado y el conjunto de sockets vigilados para agregarlo ahi

            //Si el socket del cliente conectado tiene un ID más alto que el socket con el ID más alto que tenemos hasta ahora
            if(socketsClientes[i] > mayorSocketVigilado){
                //Entonces actualizamos el socket con el ID más alto para que sea el socket del cliente conectado
                mayorSocketVigilado = socketsClientes[i];
            }
        }   

        //Select se encarga de revisar o viligar los sockets que le pasamos (el que escucha y los de cada conexión TCP activa) 
        //y se queda esperando hasta que haya actividad en alguno de esos sockets 
        //(o sea hasta que llegue una solicitud de conexión TCP al socket de escucha o hasta que llegue un mensaje de algún publisher o subscriber a través de los sockets de los clientes conectados)
        select(mayorSocketVigilado+1, &socketsVigilados, NULL, NULL, NULL);

        //Por eso hay dos casos para revisar después del select:
        //1. El caso en que haya actividad en el socket de escucha (o sea que llegue una solicitud de conexión TCP de un publisher o subscriber)
        //2. El caso en que haya actividad en alguno de los sockets de los clientes conectados (o sea que llegue un mensaje de un publisher o subscriber a través de su socket)
        
        //Caso 1: llegó una solicitud de conexión TCP al socket de escucha

        //Verificamos si el socket de escucha fue uno de los sockets que tuvo actividad
        //Si tuvo actividad es porque llegó una solicitud de conexión TCP de un cliente
        if(FD_ISSET(socketEscucha, &socketsVigilados)){
            //Aceptamos la conexión TCP del cliente que llegó 
            int nuevoSocketCliente = accept(socketEscucha, (struct sockaddr*)&direccionSocketEscucha, (socklen_t*)&tamanioDireccionSocketEscucha);
            //Agregamos el nuevo socket del cliente que se conectó al vector de socketsClientes para empezar a vigilarlo y para poder enviarle mensajes después si es un subscriber
            socketsClientes[totalClientesConectados] = nuevoSocketCliente;
            //Aun no sabemos si el cliente que se conectó es un publisher o un subscriber, por eso le ponemos 0 (indefinido por ahora) en el vector de tipoCliente
            tipoCliente[totalClientesConectados] = 0;
            //Aumentamos la cantidad de clientes conectados
            totalClientesConectados++;
            //Confirmamos que en efecto 1 cliente llegó y se pudo conectar al broker
            printf("Cliente conectado\n");
        }

        //Caso 2: llegó un mensaje de un cliente conectado (sea un publisher o un subscriber)

        //Revisamos cada cliente conectado para ver cual de ellos tuvo actividad, que puede ser:
        //1.Que envió un mensaje (en caso de los publishers) de goles o futbol en general
        //2.Que se desconectó (puede ser pasar para los publishers o los subscribers)
        //3.Que se identificó como un publisher (2) o como un subscriber (1) al enviar un mensaje al conectarse (manda: "PUBLISHER" o "SUBSCRIBER")
        for(int i=0; i<totalClientesConectados; i++){

            //Vemos si el socket de ese cliente tuvo actividad
            if(FD_ISSET(socketsClientes[i], &socketsVigilados)){

                //Guardamos la cantidad de bytes que el broker lee del mensaje que le mandó el cliente conectado (sea un publisher o un subscriber) a través de su socket
                int cantidadBytesLeidos = read(socketsClientes[i], bufferRecepcion, maxBytesFragmento); //read guarda ese mensaje en el espacio en memoria bufferRecepcion y devuelve la cantidad de bytes que leyó

                //Y vemos los tipos de actividad que pudo haber tenido ese cliente conectado:

                //Si la cantidad de bytes leidos es menor o igual a 0, significa que el cliente se desconectó
                //Por eso lo sacamos del vector de socketsClientes y del vector de tipoCliente, disminuimos la cantidad de clientes conectados y seguimos con el siguiente cliente conectado
                if(cantidadBytesLeidos <= 0){
                    close(socketsClientes[i]); //Cerramos el socket del cliente que se desconectó

                    //Como lo quitamos del vector de socketsClientes, queda un hueco o espacio vacío en esa posición del vector
                    //Para no dejar ese hueco, movemos el último socket del vector de socketsClientes a esa posición del cliente que se desconectó
                    socketsClientes[i] = socketsClientes[totalClientesConectados-1];
                    //Hacemos lo mismo para el vector de tipoCliente para no dejar un hueco ahi tampoco
                    tipoCliente[i] = tipoCliente[totalClientesConectados-1];

                    //Ahora sí disminuimos la cantidad de clientes conectados porque uno se desconectó
                    totalClientesConectados = totalClientesConectados-1;
                    //Y como en el hueco del vector de socketsClientes y de tipoCliente ahora pusimos al último cliente que estaba conectado, tenemos que revisar esa posición del vector en la siguiente iteración 
                    i = i-1;

                    //Ahora sí seguimos con el siguiente cliente conectado para revisar si tuvo actividad o no
                    continue;
                }

                //Vamos al espacio en memoria donde guardamos el mensaje que leímos del cliente conectado y le ponemos un caracter de fin
                bufferRecepcion[cantidadBytesLeidos]='\0';

                //Si el cliente que se conectó no se ha identificado todavía como publisher o subscriber, revisamos el mensaje que le mandó para ver si se identificó como publisher o subscriber
                if(tipoCliente[i]==0){ //Si es tipo indefinido, entonces vamos a revisar si es publisher o subscriber
                    
                    //Si es subscriber, le ponemos 1 en el vector de tipoCliente 
                    if(strncmp(bufferRecepcion, "SUBSCRIBER", 10)==0){
                        tipoCliente[i]=1;
                        //Confirmamos que llegó el subscriber y se identificó como tal
                        printf("Subscriber agregado\n");
                    }
                    //Si es publisher, le ponemos 2 en el vector de tipoCliente
                    else if(strncmp(bufferRecepcion, "PUBLISHER", 9)==0){
                        tipoCliente[i]=2;
                        //Confirmamos que llegó el publisher y se identificó como tal
                        printf("Publisher agregado\n");
                    }

                    //Y revisamos el siguiente cliente conectado para ver si tuvo actividad o no
                    continue;
                }

                //Si el cliente que se conectó ya se identificó como publisher o subscriber, entonces revisamos si es publisher para ver si nos mandó un mensaje de un evento de futbol 
                if(tipoCliente[i]==2){

                    //Si es publisher entonces es porque el mensaje es un evento de futbol en efecto
                    //Imprimimos ese mensaje de futbol en el broker para confirmar que si lo recibimos 
                    printf("Evento: %s\n",bufferRecepcion);

                    //Le mandamos el evento de futbol a todos los subscribers conectados (o sea a todos los clientes conectados que tengan 1 en el vector de tipoCliente)
                    for(int j=0; j<totalClientesConectados; j++){
                        if(tipoCliente[j]==1){ //Cliente conectado si es un subscriber

                            //Enviamos el mensaje a los subscribers a través de cada uno de sus sockets (que están en el vector de socketsClientes) para que lo reciban y lo impriman en su consola
                            send(socketsClientes[j], bufferRecepcion, strlen(bufferRecepcion),0); 
                        }
                    }
                }
            }
        }
    }
}
