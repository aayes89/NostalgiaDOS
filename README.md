# NostalgiaDOS
Este proyecto es una versión escrito en C puro para el motor ScummVM de aventuras gráficas.

Actualmente está pensado para correr sólo MonkeyIsland2 por lo que no incluye todas las mejoras del motor actual.<br>
Las pruebas del motor son ejecutadas en un sistema operativo sin POSIX o implementaciones complejas que garantiza el modo de video 13h (320x200x8) sin problemas.

# Logros
- Puedo pintar en gráficos 13h cuadros extraídos de archivos originales del juego.
- Manejo de los 32 opcodes necesarios para ejecutar el juego.

# Pendientes
- Optimizar el manejo del programador de instrucciones para sincronizar llamadas entre funcionalidades.
- Implementar soporte de audio iMuse real.
- Convertir en librería pura de C para facilitar integración con sistemas baremetal.
- Añadir un manual con las llamadas e instrucciones para facilitar la integración una vez sea librería.

# Capturas de pantalla
<img width="667" height="482" alt="imagen" src="https://github.com/user-attachments/assets/8b065054-367a-4abe-ba7c-d5afcaa01603" />

# Licencia
<b>MIT</b>
