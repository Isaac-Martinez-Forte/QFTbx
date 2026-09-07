# Translations of the interface

The interface is written in English, in the source code and the `.ui` forms,
and translated with Qt's tools: every text the user sees goes through
`tr()` in the GUI or `QFTBX_TR("Core", "...")` in the core, `lupdate`
collects those texts into one `.ts` file per language in this folder,
`lrelease` compiles each `.ts` into a `.qm` at build time, and the `.qm`
files go into the application's resources as `:/i18n/qftbx_<code>.qm`. The
View menu lists the system language, English, and one entry per `.qm` found
in the resources; the choice is written to the settings file
(`interface.language`, see `docs/CONFIGURATION.md`) and read at the next
start.

Nothing in the code names a language. Adding one is adding its file.

## Adding a language

1. Copy `qftbx_es.ts` to `qftbx_<code>.ts`, where `<code>` is the language
   code Qt uses (`fr`, `de`, `pt_BR`, ...), and set the `language` attribute
   of its `<TS>` element to that code. Then either delete every
   `<translation>` text so the file starts empty, or keep the Spanish as a
   reference while translating.
2. Configure a build tree with `-DQFTBX_BUILD_BENCHMARK=ON`, so the strings
   of the benchmark planner are collected too, and run

       cmake --build build --target update_translations

   which runs `lupdate` over every `.ts` in this folder and marks the texts
   without translation as unfinished.
3. Translate the texts, with Qt Linguist (`linguist qftbx_<code>.ts`) or in
   a text editor. The `%1`, `%2` placeholders are arguments filled in at run
   time and must stay; `&` before a letter marks the menu accelerator. The
   text "English" in the context `Language` is the name the View menu shows
   for the language: translate it into the language's own name ("Español",
   "Français").
4. Rebuild. The new `.qm` is picked up by the build's glob, the View menu
   shows the language under the name it gave itself, and Qt's own texts
   (the standard buttons and dialogs) come from Qt's `qtbase_<code>.qm` if
   the Qt installation has it.

The GUI test suite checks every `.ts` in this folder: a text left
unfinished or an obsolete one left behind fails it.

## Changing or adding texts

A new `tr()` or `QFTBX_TR` string, or a changed one, needs its translation
in every `.ts`: run `update_translations`, translate what it marked as
unfinished, rebuild. The test will say so otherwise.

## Vocabulary

The Spanish translation follows the interface as it was written in Spanish
before the code went English: *templates* and *boundaries* stay in English,
as the QFT literature in Spanish uses them; *lazo*, *ajuste del lazo*,
*planta*, *controlador*, *especificaciones*, *frecuencias de diseño*,
*ganancia*, *retardo*, *formato libre*, *coeficientes de polinomios*,
*ceros y polos*, *seguimiento*, *estabilidad*, *épsilon*.
