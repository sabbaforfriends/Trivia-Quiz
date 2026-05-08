#!/bin/bash

# Nome del file sorgente
SOURCE="client.c"

# Nome del file eseguibile
OUTPUT="client"

# Compilazione con gcc, opzioni -Wall e -pthread
gcc -Wall "$SOURCE" -o "$OUTPUT"

# Verifica se la compilazione è andata a buon fine
if [ $? -eq 0 ]; then
    echo "Compilazione completata con successo. Eseguibile: $OUTPUT"
else
    echo "Errore durante la compilazione."
fi
