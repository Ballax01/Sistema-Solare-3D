# Relazione progetto - Sistema Solare 3D

## Introduzione

Il progetto consiste in una simulazione 3D interattiva del Sistema Solare realizzata in C++ con SFML, OpenGL, glad e GLM.

L'obiettivo principale e stato costruire il progetto in modo incrementale, aggiungendo una funzionalita alla volta e mantenendo ogni tappa compilabile ed eseguibile. La scena finale contiene il Sole, gli 8 pianeti, la Luna, orbite ellittiche, texture, illuminazione, pannelli informativi, camera orbitale e camera libera.

## Tecnologie usate

- C++17: linguaggio principale del progetto.
- CMake: configurazione e build del progetto.
- SFML 3.0: creazione finestra, input da tastiera/mouse, ciclo principale, caricamento immagini e disegno del pannello 2D.
- OpenGL 4.1: rendering 3D della scena.
- glad: caricamento delle funzioni OpenGL.
- GLM: matrici, vettori, trasformazioni, camera e proiezioni.

## Tappa 01 - Setup progetto e finestra OpenGL

In questa tappa e stata creata la struttura iniziale del progetto CMake. Il programma apre una finestra SFML con un contesto OpenGL.

Concetti importanti:

- `sf::RenderWindow` / `sf::Window` permette di creare la finestra.
- `sf::ContextSettings` definisce profondita, stencil e versione OpenGL.
- Il ciclo principale mantiene la finestra aperta e gestisce gli eventi.

Questa tappa serve come base per tutto il resto del progetto.

## Tappa 02 - Primo rendering OpenGL

E stato aggiunto un primo rendering OpenGL semplice, utile per verificare che il contesto funzioni davvero.

In questa fase il rendering e ancora molto basilare, ma permette di capire:

- come pulire il framebuffer;
- come usare il ciclo di rendering;
- come visualizzare qualcosa nella finestra.

## Tappa 03 - Shader, VAO e VBO

Sono stati introdotti shader GLSL, VAO e VBO.

Gli shader permettono di controllare il comportamento della GPU:

- vertex shader: trasforma i vertici;
- fragment shader: calcola il colore dei pixel.

I VAO e VBO servono a inviare i dati geometrici alla GPU:

- VBO: contiene dati dei vertici;
- VAO: memorizza la configurazione degli attributi dei vertici.

Questa e una tappa fondamentale per passare da un rendering immediato o semplice a una pipeline OpenGL moderna.

## Tappa 04 - Sole e primo pianeta con GLM

Sono state introdotte le trasformazioni GLM.

Il Sole viene disegnato al centro e un primo pianeta viene posizionato tramite matrici di trasformazione.

Concetti principali:

- matrice `model`: posizione, rotazione e scala dell'oggetto;
- matrice `view`: punto di vista della camera;
- matrice `projection`: prospettiva 3D.

GLM viene usata per creare matrici di traslazione, rotazione e scala.

## Tappa 05 - Piu pianeti con orbite diverse

Sono stati aggiunti piu pianeti, ognuno con:

- distanza orbitale;
- dimensione simulata;
- velocita orbitale;
- velocita di rotazione;
- colore.

La struttura `Planet` permette di raccogliere i dati principali di ogni pianeta.

Ogni pianeta viene disegnato nello stesso ciclo, ma con parametri diversi. Questo rende il codice piu ordinato e facilmente espandibile.

## Tappa 06 - Camera orbitale controllabile

E stata aggiunta una camera orbitale controllabile.

La camera ruota intorno al centro del sistema solare usando:

- yaw: rotazione orizzontale;
- pitch: inclinazione verticale;
- distance: distanza dal centro.

La funzione di vista usa `glm::lookAt`, che costruisce la matrice `view`.

Comandi principali:

- rotazione camera;
- zoom avanti/indietro;
- limiti su pitch e distanza per evitare viste ingestibili.

## Tappa 07 - Tutti gli 8 pianeti

Sono stati aggiunti tutti gli 8 pianeti del Sistema Solare:

- Mercurio;
- Venere;
- Terra;
- Marte;
- Giove;
- Saturno;
- Urano;
- Nettuno.

Ogni pianeta ha parametri simulati per renderlo visibile e distinguibile nella scena.

La scala non e reale al 100%, perche il Sistema Solare reale ha distanze troppo grandi rispetto alle dimensioni dei pianeti.

## Tappa 08 - Orbite visibili

Sono state aggiunte le orbite visibili.

L'orbita viene generata come un insieme di punti disposti in cerchio o ellisse, poi disegnata con `GL_LINE_LOOP`.

Questa tappa aiuta a capire meglio il movimento dei pianeti e rende la scena piu leggibile.

## Tappa 09 - Selezione pianeti con mouse

E stata introdotta la selezione dei pianeti con click del mouse.

Il programma proietta la posizione 3D del pianeta sullo schermo con `glm::project`. In questo modo puo confrontare la posizione del mouse con la posizione del pianeta a schermo.

Quando un pianeta viene selezionato:

- viene memorizzato l'indice del pianeta selezionato;
- le informazioni vengono stampate nel terminale;
- il titolo della finestra viene aggiornato.

Questa tappa collega la scena 3D all'interazione utente.

## Tappa 10 - Pannello informativo SFML

Le informazioni non vengono piu mostrate solo nel terminale, ma anche dentro la finestra.

Il pannello e disegnato con SFML sopra la scena OpenGL.

Mostra:

- nome;
- tipo;
- descrizione;
- dati di scala.

Problema incontrato:

SFML Graphics usa internamente alcune chiamate OpenGL non compatibili con un contesto Core Profile. Per evitare errori OpenGL, e stato usato un contesto compatibile con SFML Graphics.

## Tappa 11 - Comandi interattivi

Sono stati aggiunti comandi extra:

- pausa/riprendi simulazione;
- aumento/diminuzione velocita;
- reset camera;
- mostra/nasconde orbite;
- mostra/nasconde interfaccia;
- selezione diretta con tastiera;
- follow del pianeta selezionato;
- reset tempo.

Questa tappa rende il progetto piu interattivo e utilizzabile durante una presentazione.

## Tappa 12 - Illuminazione base dal Sole

Sono state introdotte le normali nei vertici della sfera.

La struttura `Vertex` passa da contenere solo la posizione a contenere anche la normale.

Lo shader calcola una prima illuminazione diffusa:

- il Sole e la posizione della luce;
- la normale indica la direzione della superficie;
- il prodotto scalare tra normale e direzione della luce determina quanta luce riceve il frammento.

Risultato:

I pianeti non sono piu piatti, ma hanno un lato illuminato e un lato piu scuro.

## Tappa 13 - Descrizione e selezione del Sole

Il Sole diventa un corpo selezionabile.

Sono stati aggiunti:

- dati informativi del Sole;
- click sul Sole;
- pannello informativo dedicato;
- titolo finestra aggiornato quando il Sole e selezionato.

Questa tappa rende il Sole parte del sistema interattivo, non solo un oggetto decorativo.

## Tappa 14 - Illuminazione Phong e Sole tridimensionale

L'illuminazione e stata migliorata con il modello Phong.

Componenti:

- ambient: luce minima sempre presente;
- diffuse: luce legata all'angolo tra superficie e sorgente luminosa;
- specular: riflesso lucido verso la camera.

Il Sole usa un rendering speciale per sembrare luminoso e tridimensionale, senza essere oscurato come un pianeta.

## Tappa 15 - Texture da file

Sono state aggiunte texture reali caricate da file con `sf::Image`.

Ogni pianeta usa una texture presente in `assets/textures`.

Se una texture manca, viene usata una texture procedurale di fallback. Questo rende il progetto piu robusto e sempre eseguibile.

Concetti importanti:

- `glTexImage2D` carica i pixel nella GPU;
- `glGenerateMipmap` crea livelli di dettaglio;
- lo shader usa `sampler2D` per leggere la texture.

## Tappa 16 - Luna

E stata aggiunta la Luna come satellite della Terra.

La Luna non orbita direttamente attorno al Sole. La sua trasformazione parte dalla posizione orbitale della Terra e poi aggiunge una rotazione/traslazione locale.

Questo significa che:

- la Terra orbita attorno al Sole;
- la Luna segue la Terra;
- la Luna orbita attorno alla Terra.

La Luna e anche selezionabile con il mouse e ha una descrizione nel pannello.

## Tappa 17 - Anello di Saturno

Saturno e stato reso piu riconoscibile con un anello.

L'anello e una mesh piatta costruita con:

- raggio interno;
- raggio esterno;
- triangoli disposti a corona.

L'anello:

- segue Saturno durante l'orbita;
- e inclinato;
- usa una texture;
- ruota leggermente per dare movimento.

Problema risolto:

La prima mappatura della texture creava deformazioni. La soluzione e stata usare coordinate UV radiali, cioe far corrispondere la texture dal bordo interno al bordo esterno dell'anello.

## Tappa 18 - Atmosfera, nuvole e night map

Sono stati aggiunti layer extra sopra Terra e Venere.

Terra:

- texture principale;
- nuvole;
- night map visibile sul lato non illuminato.

Venere:

- atmosfera separata sopra la superficie.

Le nuvole e l'atmosfera ruotano con velocita leggermente diverse rispetto al pianeta, dando un effetto piu naturale.

Problema risolto:

Le sfere overlay creavano un bordo visibile. E stato ridotto usando alpha piu controllata e sfumatura verso il bordo.

## Tappa 19 - Sfondo stellato

E stato aggiunto uno sfondo stellato tramite una grande sfera texturizzata.

La sfera:

- e centrata sulla camera;
- non scrive nel depth buffer;
- non interferisce con pianeti e orbite;
- usa una texture della Via Lattea.

Questo rende la scena piu immersiva.

In questa tappa sono state anche migliorate le orbite:

- distanze piu leggibili;
- valori ispirati alle distanze reali;
- ellissi con eccentricita simulata.

## Tappa 20 - Rifinitura comandi interattivi

I comandi sono stati raffinati.

Miglioramenti:

- `A/D` per ruotare la camera orbitale;
- mouse drag per ruotare/guardare;
- rotella per zoom;
- `1` seleziona il Sole;
- `2-9` selezionano i pianeti;
- pannello informativo piu curato;
- descrizioni dei pianeti migliorate;
- aggiunta di dati reali come distanza in UA e diametro in km.

Il pannello distingue tra:

- valori reali;
- valori simulati di scena.

Questo e importante per spiegare che la scena e didattica e non in scala reale assoluta.

## Tappa 21 - Camera libera

E stata aggiunta una seconda modalita di camera.

Modalita disponibili:

- camera orbitale;
- camera libera.

Con `C` si cambia modalita.

In camera libera:

- `W/A/S/D` muovono la camera;
- `Q/E` scendono e salgono;
- `Shift` aumenta la velocita;
- il mouse permette di guardarsi intorno.

Questa tappa rende possibile esplorare la scena dall'interno, non solo guardarla da fuori.

## Tappa 22 - Documentazione finale

E stato aggiunto il file `README.md`, con:

- istruzioni di build;
- elenco comandi;
- dipendenze;
- struttura del progetto;
- riepilogo tappe;
- fonti texture;
- note tecniche;
- difficolta incontrate.

Questa relazione completa il materiale richiesto per la consegna e lo studio.

## Spiegazione generale del rendering

Il ciclo principale del programma segue questa logica:

1. legge gli eventi SFML;
2. aggiorna input e stato della simulazione;
3. aggiorna il tempo simulato;
4. calcola camera e matrici;
5. pulisce lo schermo;
6. disegna sfondo stellato;
7. disegna orbite;
8. disegna Sole;
9. disegna pianeti;
10. disegna Luna, anelli e layer atmosferici;
11. disegna pannello 2D SFML;
12. mostra il frame con `window.display()`.

## Scelte progettuali importanti

### Scala

Le distanze e dimensioni reali del Sistema Solare sono troppo diverse tra loro. Per questo il progetto usa una scala simulata per la scena e mostra i valori reali nel pannello.

### Texture

Le texture migliorano molto la riconoscibilita dei pianeti. Sono caricate da file, ma esiste anche un fallback procedurale.

### Interazione

La selezione dei corpi celesti usa la proiezione da coordinate 3D a coordinate schermo. Questo permette di cliccare un pianeta anche mentre si muove.

### Camera

Sono presenti due camere:

- orbitale, adatta a osservare il sistema;
- libera, adatta a esplorare la scena.

## Fonti texture

Le texture planetarie provengono da Solar System Scope:

- https://genesis-horizon.solarsystemscope.com/textures/

Attribuzione/licenza consultata anche tramite:

- https://felgo.com/doc/qt/qt3d-attribution-solar-system-scope/

## Possibili miglioramenti futuri

- Aggiungere piu lune per Giove e Saturno.
- Aggiungere asteroidi o fascia principale.
- Separare il codice in classi (`Planet`, `Camera`, `Shader`, `TextureManager`).
- Aggiungere una GUI piu completa.
- Migliorare la precisione fisica delle orbite.
- Usare normal map/specular map per rendere le superfici piu realistiche.

## Conclusione

Il progetto finale e una simulazione 3D interattiva che integra vari argomenti di computer grafica:

- pipeline OpenGL moderna;
- shader GLSL;
- trasformazioni con matrici;
- illuminazione;
- texture;
- gestione input;
- camera;
- rendering 2D sovrapposto;
- organizzazione incrementale con Git.

Ogni tappa aggiunge una funzionalita autonoma e mantiene il progetto compilabile, rendendo il percorso di sviluppo chiaro e presentabile.
