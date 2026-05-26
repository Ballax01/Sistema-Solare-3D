# Sistema Solare 3D

Progetto d'esame di Fondamenti di Computer Grafica: simulazione interattiva di un sistema solare 3D in C++.

Il progetto mostra il Sole, gli 8 pianeti, la Luna, orbite ellittiche, texture reali, illuminazione Phong, anello di Saturno, atmosfera/nuvole della Terra e di Venere, sfondo stellato e due modalita di camera.

## Tecnologie

- C++17
- CMake
- SFML 3.0 per finestra, input, ciclo principale, immagini e testo 2D
- OpenGL 4.1 con glad
- GLM per matrici, trasformazioni e calcoli matematici

Il progetto e stato sviluppato su Windows.

## Struttura

```text
sistema-solare-3d/
  CMakeLists.txt
  README.md
  Relazione_SistemaSolare3D.docx
  src/
    main.cpp
    glad.c
  include/
    glad/
    KHR/
    glm/
  assets/
    textures/
      sun.jpg
      mercury.jpg
      venus.jpg
      venus_atmosphere.jpg
      earth.jpg
      earth_clouds.jpg
      earth_nightmap.jpg
      mars.jpg
      jupiter.jpg
      saturn.jpg
      saturn_ring.png
      uranus.jpg
      neptune.jpg
      moon.jpg
      stars_milky_way.jpg
  scripts/
    build-tappe.ps1
```

## Build

Il file `CMakeLists.txt` non contiene path assoluti. Se SFML non viene trovato automaticamente, indicare a CMake la cartella di configurazione di SFML:

```powershell
cmake -S . -B build -DSFML_DIR="C:/libs/SFML-3.0.0/lib/cmake/SFML"
cmake --build build --config Debug
```

In alternativa si puo indicare la cartella principale di SFML:

```powershell
cmake -S . -B build -DSFML_ROOT="C:/libs/SFML-3.0.0"
cmake --build build --config Debug
```

Eseguibile generato:

```text
build/Debug/SistemaSolare3D.exe
```

Il post-build copia automaticamente le DLL di SFML nella cartella dell'eseguibile.

## Tappe di sviluppo

Le tappe di sviluppo sono versionate tramite tag Git da `tappa-01` a `tappa-23`.

Per vedere le tappe disponibili:

```powershell
git tag --list "tappa-*"
```

Per compilare tutte le tappe con un solo comando:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-tappe.ps1 -SfmlDir "C:/libs/SFML-3.0.0/lib/cmake/SFML"
```

Gli eseguibili vengono generati nelle sottocartelle di `build/tappe/`.

## Comandi

```text
Mouse sinistro click       Seleziona Sole, Luna o pianeti
Mouse sinistro trascina    Ruota/guarda con la camera
Rotella mouse              Zoom in camera orbitale, movimento avanti/indietro in camera libera
C                          Cambia tra camera orbitale e camera libera

Camera orbitale:
A / D                      Ruota attorno alla scena
W / S                      Zoom avanti/indietro

Camera libera:
W / A / S / D              Movimento avanti/sinistra/indietro/destra
Q / E                      Scendi/sali
Shift                      Movimento piu veloce

SPACE                      Pausa/riprendi simulazione
+ / -                      Aumenta/diminuisce velocita simulazione
T                          Reset tempo simulazione
R                          Reset camera
O                          Mostra/nasconde orbite
I                          Mostra/nasconde interfaccia 2D
F                          Esce dal follow del corpo selezionato
1                          Seleziona Sole
2-9                        Seleziona Mercurio-Nettuno
ESC                        Chiude
```

## Funzionalita principali

- Finestra SFML con contesto OpenGL.
- Rendering con shader GLSL, VAO, VBO ed EBO.
- Sfere generate proceduralmente.
- Sole centrale texturizzato e leggermente animato.
- 8 pianeti con texture, rotazione e rivoluzione.
- Distanze di scena ispirate alle distanze reali in UA, compresse per rendere la scena esplorabile.
- Orbite ellittiche con eccentricita simulata.
- Selezione di Sole, Luna e pianeti con mouse o tastiera.
- Pannello informativo SFML con caratteristiche reali e scala di scena.
- Camera orbitale e camera libera.
- Luna in orbita attorno alla Terra.
- Anello texturizzato di Saturno, inclinato e in leggera rotazione.
- Nuvole, night map terrestre e atmosfera di Venere.
- Sfondo stellato con texture.
- Illuminazione Phong per i pianeti.

## Note su scala e realismo

Le distanze reali del Sistema Solare non sono direttamente rappresentabili in una scena compatta: se fossero in scala reale, i pianeti interni sarebbero troppo vicini al Sole e quelli esterni troppo lontani.

Per questo motivo il progetto separa:

- valori reali, mostrati nel pannello informativo, come distanza media in UA e diametro in km;
- valori di scena, usati per rendere il sistema visibile, navigabile e didattico.

Anche le eccentricita orbitali sono simulate: seguono l'idea delle orbite reali, ma sono adattate per essere percepibili graficamente.

## Tappe Git

```text
tappa-01  Setup progetto e finestra OpenGL
tappa-02  Primo rendering OpenGL
tappa-03  Shader, VAO e VBO
tappa-04  Sole e primo pianeta con GLM
tappa-05  Piu pianeti con orbite diverse
tappa-06  Camera orbitale controllabile
tappa-07  Tutti gli 8 pianeti
tappa-08  Orbite visibili
tappa-09  Selezione pianeti con mouse
tappa-10  Pannello informativo SFML
tappa-11  Comandi interattivi
tappa-12  Illuminazione base dal Sole
tappa-13  Descrizione e selezione del Sole
tappa-14  Illuminazione Phong e Sole tridimensionale
tappa-15  Texture da file
tappa-16  Luna in orbita attorno alla Terra
tappa-17  Anello di Saturno
tappa-18  Atmosfera, nuvole e night map
tappa-19  Sfondo stellato
tappa-20  Rifinitura comandi interattivi
tappa-21  Camera libera
tappa-22  Documentazione finale e relazione
tappa-23  Relazione dettagliata del progetto
```

Per controllare i tag:

```powershell
git tag --list "tappa-*"
git log --oneline --decorate --all
```

## Texture e fonti

Le texture planetarie usate nel progetto provengono da Solar System Scope:

- Solar System Scope Textures: https://genesis-horizon.solarsystemscope.com/textures/
- Nota di attribuzione/licenza riportata anche nella documentazione Qt 3D: https://felgo.com/doc/qt/qt3d-attribution-solar-system-scope/

Texture usate:

- Sole
- Mercurio
- Venere
- Atmosfera di Venere
- Terra
- Nuvole terrestri
- Night map terrestre
- Luna
- Marte
- Giove
- Saturno
- Anello di Saturno
- Urano
- Nettuno
- Sfondo stellato / Via Lattea

Se una texture non viene trovata, il programma genera una texture procedurale di fallback per mantenere il progetto eseguibile.

## Difficolta incontrate

- Integrazione tra OpenGL moderno e SFML Graphics: il profilo OpenGL Core causava errori con alcune chiamate usate internamente da SFML per il rendering 2D. La soluzione e stata usare un contesto compatibile con SFML Graphics.
- Disegno dell'interfaccia 2D sopra la scena 3D: e stato necessario gestire gli stati OpenGL/SFML e aggiornare la view 2D dopo il resize della finestra.
- Texture dell'anello di Saturno: la prima mappatura UV produceva deformazioni visibili; la soluzione e stata usare coordinate UV radiali.
- Scala del sistema solare: le distanze reali sono state compresse per mantenere il progetto navigabile.
- Overlay di nuvole e atmosfera: le sfere aggiuntive creavano un bordo visibile; e stato ridotto con alpha e dimensioni piu controllate.
- Distinzione tra click e trascinamento mouse: e stata introdotta una soglia di movimento per evitare selezioni involontarie durante la rotazione della camera.

## Note sul codice

Il progetto e concentrato in `src/main.cpp` per mantenere semplice la struttura richiesta. Alcune parti potrebbero essere separate in classi o moduli in un progetto piu grande, ma per un progetto d'esame a tappe la struttura monofile rende piu facile seguire l'evoluzione.

Lo sviluppo e stato svolto in modo incrementale con supporto di assistente AI per organizzare modifiche, debug e documentazione.

## Stato finale

Il risultato finale e un sistema solare 3D interattivo con:

- Sole;
- 8 pianeti;
- Luna;
- orbite ellittiche;
- texture reali;
- illuminazione Phong;
- anello di Saturno;
- nuvole e atmosfera;
- sfondo stellato;
- selezione dei corpi celesti;
- pannello informativo;
- camera orbitale;
- camera libera;
- cronologia Git organizzata in tappe.
