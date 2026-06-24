# RNBO Synth — JUCE + RNBO

Petit synthé monophonique : un **clavier MIDI virtuel** (JUCE) qui envoie les
notes à un **moteur RNBO** (`cycle~` + enveloppe `line~`).

```
┌─────────────────────────┐     MIDI      ┌──────────────────────────────┐
│ MidiKeyboardComponent    │ ───notes───▶ │ RNBO patch (SimpleSynth)      │
│ (clavier à l'écran, JUCE)│              │ notein → mtof → cycle~ → *~    │
└─────────────────────────┘              │ note-on → line~ (ADSR) ───┘    │
                                          │                    → out~ 1    │
                                          └──────────────────────────────┘
```

## Le patch RNBO

`patch/SimpleSynth.rnbopat` contient :

| Objet                 | Rôle                                                        |
|-----------------------|------------------------------------------------------------|
| `notein`              | reçoit les notes MIDI (pitch, vélocité)                     |
| `mtof`                | note MIDI → fréquence en Hz                                 |
| `cycle~`              | oscillateur sinus                                          |
| `sel 0`               | détecte note-on (vélocité ≠ 0) pour déclencher l'enveloppe |
| `1 100 1 500 0 200`   | message d'enveloppe envoyé à `line~`                        |
| `line~`               | génère l'enveloppe (ADSR)                                  |
| `*~`                  | applique l'enveloppe à l'oscillateur                        |
| `out~ 1`              | sortie audio                                                |

### À propos de l'enveloppe `line~`

Tu avais écrit `(1, 100 1 500 0 200)`. En syntaxe `line~`, une liste se lit par
**paires `cible durée`** :

| Segment        | Effet                                       |
|----------------|---------------------------------------------|
| `1 100`        | monte à 1 en 100 ms  → **Attack**           |
| `1 500`        | reste à 1 pendant 500 ms → **Decay/Sustain**|
| `0 200`        | redescend à 0 en 200 ms → **Release**       |

⚠️ J'ai **retiré la virgule** de `1,` : dans un message Max/RNBO, la virgule
sépare deux messages distincts. `1,` enverrait d'abord un saut instantané à 1
(sans rampe), ce qui casserait l'attaque de 100 ms. Le message
`1 100 1 500 0 200` (sans virgule) donne l'enveloppe décrite ci-dessus.
Si tu voulais vraiment le comportement séquencé, remets la virgule dans le
message — tout le reste fonctionne pareil.

## Étape 1 — Exporter le C++ depuis RNBO (Max 9)

1. Ouvre `patch/SimpleSynth.rnbopat` dans RNBO.
2. Panneau **Export → C++ Source Code Export**.
3. Destination = le dossier **`export/`** de ce projet.
4. **Coche `Export RNBO C++ Library`** (indispensable : ça copie `RNBO.h` et la
   librairie dans `export/rnbo/`).
5. Exporte.

Tu dois obtenir au minimum :

```
export/SimpleSynth.rnbopat.cpp   # le patcher, nommé d'après le patch
export/rnbo/RNBO.cpp             # l'amalgame de la librairie RNBO (compilé)
export/rnbo/RNBO.h
export/rnbo/...
```

Le CMake détecte automatiquement le `.cpp` du patcher (peu importe son nom) et
compile `rnbo/RNBO.cpp` — rien à éditer si tu changes de patch.

## Étape 2 — Compiler (CMake)

Depuis le dossier du projet :

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE est téléchargé automatiquement (FetchContent, tag 8.0.4) — pas besoin de
l'installer à la main.

## Étape 3 — Lancer

L'app standalone se trouve dans `build/` :

- **macOS** : `build/RNBOSynthJuce_artefacts/Release/RNBO Synth.app`
- **Windows** : `build/RNBOSynthJuce_artefacts/Release/RNBO Synth.exe`
- **Linux** : `build/RNBOSynthJuce_artefacts/Release/RNBO Synth`

Clique sur les touches du clavier à l'écran → tu entends le synthé.
(Tu peux aussi brancher un clavier MIDI matériel : il est pris en compte par le
device audio/MIDI par défaut de JUCE.)

## Structure

```
RNBOSynthJuce/
├── CMakeLists.txt          # build + récup JUCE + compile l'export RNBO
├── README.md
├── patch/
│   └── SimpleSynth.rnbopat # le patch à ouvrir dans RNBO
├── export/                 # ← l'export C++ de RNBO va ici (étape 1)
└── Source/
    ├── Main.cpp            # fenêtre / app JUCE
    ├── MainComponent.h     # clavier MIDI + RNBO::CoreObject
    └── MainComponent.cpp   # MIDI → RNBO → audio
```

## Notes techniques

- L'intégration suit le même schéma que `rnbo.example.juce` officiel : le `.cpp`
  du patcher (ex. `SimpleSynth.rnbopat.cpp`) et `rnbo/RNBO.cpp` sont compilés dans
  le binaire ; le patcher expose `GetPatcherFactoryFunction()` et le constructeur
  par défaut de `RNBO::CoreObject` charge ce patcher.
- Les buffers audio RNBO (`RNBO::SampleValue`, `double` ou `float` selon
  l'export) sont convertis vers les buffers `float` de JUCE dans
  `getNextAudioBlock`.
- Le patch a une seule sortie (`out~ 1`) → le signal mono est recopié sur les
  deux canaux stéréo.
- Pour un vrai polyphonique, il faudrait passer le moteur en `poly` côté RNBO ;
  ici c'est volontairement monophonique (« simple »).
```
