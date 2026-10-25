# MockupCppConverter

Windows-Werkzeug, das eine HTML-Mockup-Datei analysiert und eine C++-Startvorlage für eine visuell originalgetreue Umsetzung erzeugt.

## Ansatz: Fidelity-Modus

Tailwind, moderne CSS-Layouts und JavaScript lassen sich nicht verlustfrei in klassische Win32-Controls übersetzen. Deshalb erzeugt der Converter zwei zusammengehörige Dateien:

- `<name>_template.cpp`: kompakter, ohne zusätzliche Linker-Optionen kompilierbarer C++-Launcher.
- `mockup.html`: unveränderte visuelle Quelle des Mockups.

Die erzeugte EXE startet das HTML über das installierte Microsoft Edge im App-Modus. Damit bleiben CSS, Tailwind, externe Schriftarten, Icons und Interaktionen erhalten.

## Voraussetzungen

- Windows 10/11
- Microsoft Edge (standardmäßig installiert)
- Ein C++-Compiler, etwa die KR-Compiler Toolchain

## Verwendung

1. Starte `MockupCppConverter.exe`.
2. Wähle eine `.html`- oder `.htm`-Mockup-Datei und einen Ausgabeordner.
3. Klicke **C++ Vorlage erzeugen**.
4. Kompiliere die erzeugte `.cpp` direkt mit dem KR-Compiler.
5. Lege die erzeugte EXE neben `mockup.html` ab und starte sie.

Beispiel:

```bat
g++ aura_workspace_mockup_template.cpp -o aura_workspace.exe
```

## Den Converter selbst bauen

```bat
build_with_kr_compiler.bat
```

Der Build nutzt `H:\KR-Compiler\KR-Compiler-Windows-x64-v4.3.1\toolchain\bin\g++.exe` und erzeugt eine statisch gelinkte Converter-EXE.

## Übergabe an KI-Agenten

Gib einem Agenten immer den gesamten erzeugten Ordner oder mindestens diese beiden Dateien: `<name>_template.cpp` und `mockup.html`. Eine ausführliche Arbeitsanweisung ist in [AGENTS.md](AGENTS.md) enthalten.

## Einschränkungen

Der Fidelity-Modus ist absichtlich browserbasiert. Er ist eine verlässliche Design-Referenz für KI-Agenten; die schrittweise Ersetzung durch eine vollständig native GUI ist ein separater Entwicklungsauftrag.
