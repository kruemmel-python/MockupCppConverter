# Anleitung für KI-Agenten

## Ziel

Arbeite auf Basis eines von MockupCppConverter generierten Ordners. `mockup.html` ist die verbindliche visuelle Referenz; sie darf nicht durch vereinfachte, anders aussehende Platzhalter ersetzt werden.

## Eingaben

- `<name>_template.cpp`: C++-Einstiegspunkt, der das Referenz-Mockup startet.
- `mockup.html`: verbindliches Layout, CSS, Tailwind-Klassen, Texte und Interaktionen.
- `IMPLEMENTATION_MAP.json`: maschinenlesbarer Backlog der erkannten Controls; jedes Element enthält `id`, `event`, `required_handler` und `status`.
- `AI_HANDOFF.md`: vollständige Ein-Datei-Übergabe mit allen vorgenannten Artefakten. Verwende sie, wenn nur eine Datei übergeben werden kann.

Alle Dateien gehören zusammen. Eine C++-Datei allein enthält bewusst keine vollständige CSS-Rendering-Engine.

## Arbeitsregeln

1. Lies zuerst `IMPLEMENTATION_MAP.json`; nutze die HTML-Datei nur zur visuellen und semantischen Prüfung der aufgeführten Controls.
2. Erhalte Geometrie, Abstände, Farben, Typografie, sichtbare Texte und responsive Regeln.
3. Verwende das laufende `mockup.html` als visuellen Soll-Zustand für jeden Vergleich.
4. Implementiere fachliche Logik in kleinen, testbaren Schritten; ersetze den Fidelity-Modus nur, wenn die native Ansicht visuell geprüft ist.
5. Ergänze keine zusätzlichen Bibliotheken ohne ausdrückliche Freigabe.
6. Liefere vollständige, kompilierbare C++-Dateien und nenne die nötigen Build-Schritte.

## Empfohlener Übergabe-Prompt

```text
Erstelle die Anwendung in C++ auf Basis der angehängten Dateien.
`mockup.html` ist die verbindliche Design- und Interaktionsreferenz.
Bewahre Layout, Farben, Typografie, Texte und sichtbare Controls.
`<name>_template.cpp` ist der vorhandene Einstiegspunkt.
Ersetze Platzhalter schrittweise durch echte Logik und gib vollständigen, kompilierbaren Code aus.
Verwende keine Drittanbieter-Abhängigkeiten ohne Rückfrage.
```
