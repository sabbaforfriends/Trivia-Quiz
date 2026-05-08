//Qui implemento il protocollo di comuncazione
//1. invia il tipo
//2. prima del messaggio effettivo viene scambiata la lunghezza del messaggio
//3. poi il messaggio effettivo
//4. infine una sorta di ACK


int my_send(int socket, const char* t_buffer, const char* buffer) {
    uint32_t len = strlen(buffer);  // lunghezza del messaggio
    uint32_t net_len = htonl(len);  // conversione in network byte order



    // 1. Invio il tipo
    if(send(socket, (void*)t_buffer, CMD_LEN, 0) != CMD_LEN){
        return -1;
    }
    // 2. invio la lunghezza
    if (send(socket, &net_len, sizeof(net_len), 0) != sizeof(net_len)) {
        return -1;
    }

    // 3. Invio il messaggio
    if (send(socket, (void*)buffer, len, 0) != len) {
        return -1;
    }

    // 4. Ricevo ACK
    char ack_buffer[ACK_LEN + 1] = {0};
    if (recv(socket, ack_buffer, ACK_LEN, 0) != ACK_LEN) {
        return -1;
    }

    if (strncmp(ack_buffer, ACK_MSG, ACK_LEN) != 0) {
        return -1;
    }

    return 0;
}

int my_recv(int socket, char** t_buffer, char** buffer) {
    uint32_t net_len;
    *t_buffer = (char*)malloc(CMD_LEN + 1);
    if (!*t_buffer) {
        perror("malloc fallita");
        return -1;
    }
    // 1. ricevo il tipo
    if(recv(socket, *t_buffer, CMD_LEN, 0)!=CMD_LEN){
        free(*t_buffer);
        return -1;
    }
    
    // 2. Ricevo la lunghezza
    if (recv(socket, &net_len, sizeof(net_len), 0) != sizeof(net_len)) {
        return -1;
    }

    uint32_t len = ntohl(net_len);  // conversione da network byte order

    // 3. Alloco e ricevo il messaggio
    *buffer = (char*)malloc(len + 1);
    if (!*buffer) {
        printf("malloc fallita");
        return -1;
    }

    if (recv(socket, *buffer, len, 0) != len) {
        free(*buffer);
        return -1;
    }
    (*buffer)[len] = '\0';  // terminazione stringa

    // 4. Invio ACK
    if (send(socket, (void*)ACK_MSG, ACK_LEN, 0) != ACK_LEN) {
        free(*buffer);
        return -1;
    }

    return 0;
}


// funzione per la rimozione degli spazi in input
void trim_edges(char *str){
    int start = 0;
    int end = strlen(str) - 1;

    // Trova il primo carattere non spazio
    while (isspace((unsigned char)str[start])) {
        start++;
    }

    // Trova l'ultimo carattere non spazio
    while (end > start && isspace((unsigned char)str[end])) {
        end--;
    }

    // Sposta la sottostringa al principio
    int i = 0;
    while (start <= end) {
        str[i++] = str[start++];
    }

    str[i] = '\0'; // Terminatore di stringa
}
