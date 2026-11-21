# FretScope

Plugin VST3 / AU / Standalone qui affiche en temps réel sur un manche de **guitare** ou de **basse** les notes MIDI reçues, avec la gamme de ton choix en surimpression.

FretScope ne produit aucun son : il lit le MIDI et le visualise.

## Fonctionnalités

- **Deux instruments** : guitare (6 cordes, E A D G B E) et basse (4 cordes, E A D G), 24 frettes.
- **Gamme en surimpression** : fondamentale + 17 gammes (majeure, mineures, modes, pentatoniques, blues, demi-ton/ton, etc.). La fondamentale est en orange.
- **Notes jouées** : en cyan si elles appartiennent à la gamme, en rouge sinon.
- **Étiquettes** : degrés (`1 b3 5 b7`…) ou noms de notes.
- **Mode Hex MIDI** : pour les guitares MIDI hexaphoniques (un canal par corde, canal 1 = corde la plus aiguë). Chaque note n'allume alors que sa corde. Sans ce mode, une note s'allume à toutes ses positions sur le manche.
- **Lecture en direct** : notes tenues, degrés par rapport à la fondamentale et intervalles entre notes.
- Interface redimensionnable, paramètres sauvegardés avec le projet du DAW.

## Utilisation

1. Charge FretScope sur une piste instrument de ton DAW.
2. Envoie-lui du MIDI : clavier MIDI, guitare MIDI, ou plugin de pitch-to-MIDI placé avant (pour une guitare ou une basse classique).
3. Choisis l'instrument, la fondamentale et la gamme.

## Compilation

Prérequis : CMake ≥ 3.22 et un compilateur C++17 (Visual Studio 2022, Xcode, GCC/Clang). JUCE est téléchargé automatiquement.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Les binaires sont générés dans `build/FretScope_artefacts/Release/` (`VST3/`, `AU/` sur macOS, `Standalone/`).

### Option effet MIDI

Par défaut, le plugin se déclare comme instrument silencieux, ce qui est accepté par tous les DAW. Pour obtenir un vrai effet MIDI (le MIDI traverse le plugin) :

```bash
cmake -B build -DFRETSCOPE_MIDI_FX=ON
```

### Linux : dépendances

```bash
sudo apt-get install libasound2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev
```

## Installation

Copie le dossier `FretScope.vst3` entier dans le dossier VST3 système, puis relance le scan de plugins du DAW :

| Système | Dossier |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` (AU : `~/Library/Audio/Plug-Ins/Components/`) |
| Linux | `~/.vst3/` |

Les binaires ne sont pas signés : macOS demandera une autorisation au premier chargement.

## Intégration continue

Le workflow `.github/workflows/build.yml` compile le plugin pour Windows, macOS (arm64 + Intel) et Linux à chaque push. Les binaires sont disponibles dans les artefacts de l'onglet *Actions*.

## Structure

```
CMakeLists.txt
Source/
  Theory.h             gammes, intervalles, accordages
  PluginProcessor.*    lecture du MIDI, paramètres, état
  PluginEditor.*       interface et dessin du manche
.github/workflows/     build multi-plateforme
```

## Technologies

[JUCE](https://juce.com) 8, C++17, CMake.
