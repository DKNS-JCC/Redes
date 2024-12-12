#!/bin/bash

PID=$(pgrep -f "./servidor")
if [ -n "$PID" ]; then
  echo "Matando el proceso ./servidor con PID $PID..."
  kill $PID
  if [ $? -eq 0 ]; then
    echo "Proceso ./servidor detenido con éxito."
  else
    echo "Error al intentar detener el proceso ./servidor."
    exit 1
  fi
else
  echo "No se encontró ningún proceso llamado ./servidor."
fi

echo "Ejecutando make..."
make
if [ $? -ne 0 ]; then
  echo "Error al ejecutar make."
  exit 1
fi

echo "Ejecutando ./servidor..."
./servidor &
if [ $? -eq 0 ]; then
  echo "./servidor se está ejecutando correctamente."
else
  echo "Error al intentar ejecutar ./servidor."
  exit 1
fi