# NostalgiaDOS
Este proyecto es una versión escrito en C puro para el motor ScummVM de aventuras gráficas.

Está pensado para correr sólo "The Day of the Tentacle" (DOTT) por lo que no incluye todas las mejoras del motor ScummVM.<br>
Actualmente sólo puede ejecutar juegos para la versión 6 del motor Scumm. Entre los juegos disponibles se encuentran:<br>

# Juegos
- The Day of the Tentacle (DOTT)
- Sam & Max Hit the Road
- Indiana Jones and the Fate of Atlantis
- The Dig (Aunque usa la versión 7, con algunos ajustes es posible)

Las pruebas del motor son ejecutadas en un sistema operativo sin POSIX o implementaciones complejas que garantiza el modo de video 13h (320x200x8) sin problemas.

# Logros
- Puedo pintar en gráficos 13h cuadros extraídos de archivos originales del juego.
- Manejo de los opcodes necesarios para ejecutar el juego.
- Soporte de mouse y teclado.
- Añadida librería propia a la maquina virtual para operar de forma independiente.

# Pendientes
- Optimizar el manejo del programador de instrucciones para sincronizar llamadas entre funcionalidades.
- Implementar soporte de audio iMuse real.
- Actualizar el motor para dar soporte a la versión 4 y 5 de Scumm. (mucho trabajo, cambian mucho o todo, entre ellas)
- Convertir en librería pura de C para facilitar integración con sistemas baremetal. (hecho)
- Añadir un manual con las llamadas e instrucciones para facilitar la integración una vez sea librería.

# Juegos pendientes
- MonkeyIsland 1
- MonkeyIsland 2 (hecho)

# Capturas de pantalla
<img width="667" height="482" alt="imagen" src="https://github.com/user-attachments/assets/8b065054-367a-4abe-ba7c-d5afcaa01603" />

<img width="642" height="466" alt="imagen" src="https://github.com/user-attachments/assets/5de0bc2e-48fc-4821-99dd-41340d9d5edd" />

<img width="641" height="462" alt="imagen" src="https://github.com/user-attachments/assets/edf5e425-e185-419b-bb0c-c97107c4da4b" />

<img width="719" height="460" alt="imagen" src="https://github.com/user-attachments/assets/a114af69-c436-4490-8601-df7473ac6b6a" />



# Licencia

MIT License

Copyright (c) 2026 Allan (Slam)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
