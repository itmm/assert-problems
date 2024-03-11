<!-- vim: set spelllang=de noexpandtab: -->
# Das Problem mit `assert()`

In diesem Blog halte ich meine Gedanken zu Programmiersprachen fest.
Hauptsächlich werden das erstmal C++ und Oberon sein. Mit diesen Sprachen
befasse ich mich im Augenblick am intensivsten. Das Ziel ist es, kleine
Handreichungen zu geben, um stabilere und bessere Programme zu entwickeln.
Die Sprachen sind riesig und gerade C++ hat einen sehr starken Fokus auf
Geschwindigkeit. Dafür werden oft Sicherheit und Stabilität geopfert. Auf
einer schwankenden Plattform kann aber nur sehr schwierig etwas Stabiles
errichtet werden.

Daher ist der Sinn dieser Beiträge, das Fundament zu festigen. Damit können 
wir leichter gute Programme schreiben. Wenn sie dann etwas langsamer Laufen,
ist das ein geringer Preis für die resultierende Qualitätssteigerung.


## Wann sollte ich `assert()` verwenden

Das Makro `assert` ist in C++ ein Überbleibsel aus den C Tagen. Ich verwende
ein kleines Programm als durchgehendes Beispiel: die Ermittlung der Länge
einer Zeichenfolge. Hier zuerst die Implementierung in der Datei `strlen.cpp`:

```c++
#include <cassert>
#include <cstddef>

[[nodiscard]] size_t strlen(const char* str) {
	assert(str);
	size_t count { 0 };
	while (*str++) { ++count; }
	return count;
}
```

Ein kleines Test-Programm `t_strlen.cpp` dafür sieht so aus:

```c++
#include <cassert>

#include "strlen.h"

int main() {
	assert(strlen("") == 0);
	assert(strlen("abc") == 3);
	assert(strlen("a\0b") == 1);
}
```

Damit der Test baut, brauche ich noch den Header `strlen.h`:

```c++
#pragma once

#include <cstddef>

[[nodiscard]] size_t strlen(const char* a);
```

In diesem Progrämmchen benutze ich `assert` für zwei unterschiedliche
Aufgaben:

1. Ich prüfe in der Funktion `strlen`, ob der übergebene Zeiger `nullptr`
   ist und

2. Ich verwende in `t_strlen.cpp` für einfache Unit-Tests.

Beide Anwendungen habe ich in der freien Wildbahn oft gesehen. Es handelt
sich also nicht um ein konstruiertes Problem.

Gerade der erste Fall macht jedoch richtig Probleme: Was passiert, wenn ich
eine Release-Version baue? In diesem Fall wird das `assert` einfach
weggelassen. Falls `strlen` mit einem `nullptr` aufgerufen wird, kann alles
Mögliche passieren. In der Entwicklungsumgebung treten diese Probleme nicht
auf.

Es ist ein bekanntes Problem, dass die Release-Version sich nicht von der
Debug-Version unterscheiden sollte. Es ist jedoch keine Lösung, mit der
Debug-Version in die Produktion zu gehen: oftmals fehlen dort die
Optimierungsschritte, die für einen schnellen Ablauf notwendig sind.

Ich sollte also die Release-Version testen und in der habe ich kein `assert`
mehr. Klar, ich kann es auch dort einbinden, aber das ist dann kein
Standard-C++ mehr. Eine Lösung kann es sein, den Test noch einmal
zusätzlich einzubauen. Ich habe `strlen.cpp` entsprechend angepasst:

```c++
// ...
[[nodiscard]] size_t strlen(const char* str) {
	assert(str);
	if (! str) { return 0; }
	// ...
}
```


## Eine Exception werfen

Das Verhalten ist besser, aber noch nicht gut. Zumindest passiert jetzt nichts
Schlimmes, wenn wir die Funktion mit `nullptr` aufrufen. Aber es wird auch
kein Fehler zurückgeliefert. Ich kann natürlich argumentieren, dass `0` die
perfekt richtige Antwort für den Aufruf mit `nullptr` ist. Aber dann
bräuchte ich den `assert` gar nicht.

Ich will mit dem `assert` ja gerade ausdrücken, dass es ein Fehler ist, die
Funktion mit `nullptr` aufzurufen. Vielleicht ist eine Exception für diesen
Fehlerfall besser geeignet. Ich habe `strlen.cpp` wie folgt angepasst:

```c++
// ...
#include <cstddef>
#include <stdexcept>
// ...
[[nodiscard]] size_t strlen(const char* str) {
	assert(str);
	if (! str) { throw std::invalid_argument { "must not be nullptr" }; }
	#if 0
	if (! str) { return 0; }
	#endif
	// ...
}
```

Das `#if 0` ist ein Workaround für mein Program
[`md-patcher`](https://github.com/itmm/md-patcher). Es extrahiert den
Source-Code aus Markdown-Dokumenten und setzt ihn zusammen. Momentan gibt
es keine Möglichkeit, bestehende Zeilen zu löschen. Wenn Sie jedoch mit
einem `#if 0` eingeklammert werden, werden Sie nicht mehr generiert. Der
resultierende Code ist sauber. Im Markdown sieht es jedoch nicht so toll
aus.

Im nächsten Schritt, packe ich das Werfen der Exception in eine eigene
Funktion, die dann `assert` ersetzen kann. Hier ist dazu der Header
`require.h`:

```c++
#pragma once

#include <exception>
#include <string>

class Require_Error: public std::exception {
	public:
		Require_Error(const std::string& what): what_ { what } { }
		const char* what() const noexcept override {
			return what_.c_str();
		}

	private:
		std::string what_;
	
};

#define require(...) do { if (!(__VA_ARGS__)) { \
	require_failed(__FILE__, __LINE__, #__VA_ARGS__) } } while (false)

#define require_failed(FILE, LINE, EXPR) require_failed_2(FILE, LINE, EXPR)

#define require_failed_2(FILE, LINE, EXPR) \
	std::string what { FILE ":" }; \
	what += #LINE; \
	what += " assertion failed: "; \
	what += EXPR; \
	throw Require_Error { what };
```

Damit kann ich dann die Funktion in `strlen.cpp` kompakter beschreiben:

```c++
#if 0
#include <cassert>
#endif
#include <cstddef>
#if 0
#include <stdexcept>
#endif

#include "require.h"

[[nodiscard]] size_t strlen(const char* str) {
	#if 0
	assert(str);
	if (! str) { throw std::invalid_argument { "must not be nullptr" }; }
	// ...
	#endif
	require(str);
	// ...
}
```

Die gleiche Anpassung gilt für `t_strlen.cpp`:

```c++
#if 0
#include <cassert>
#endif

#include "require.h"
#include "strlen.h"

int main() {
	#if 0
	assert(strlen("") == 0);
	assert(strlen("abc") == 3);
	assert(strlen("a\0b") == 1);
	#endif
	require(strlen("") == 0);
	require(strlen("abc") == 3);
	require(strlen("a\0b") == 1);
}
```

Mit dem registrieren eines globalen Handlers kann ich die Ausgabe noch ein
wenig verbessern. Dazu greife ich in `require.h` auf eine globale Variable
zu (um sie auch sicher zu verwenden):

```c++
// ...
#include <string>

class Global_Require_Handler {
	public:
		Global_Require_Handler();
		const std::string& operator()(const std::string& what) {
			return what;
		}
};

class Require_Error: public std::exception {
	public:
		#if 0
		Require_Error(const std::string& what): what_ { what } { }
		#endif
		Require_Error(const std::string& what):
			what_ { handler_(what) } { }
		// ...

	private:
		std::string what_;
		static Global_Require_Handler handler_;
	
};
// ...
```

In der Datei `require.cpp` initialisiere ich den Handler und registriere
dabei einen Exception-Handler:

```c++
#include <iostream>

#include "require.h"

Global_Require_Handler::Global_Require_Handler() {
	std::set_terminate([]() -> void {
		try {
			std::rethrow_exception(std::current_exception());
		}
		catch (const Require_Error& err) {
			std::cerr << err.what() << "\n";
			std::exit(EXIT_FAILURE);
		}
	});
}

Global_Require_Handler Require_Error::handler_ { };
```

Damit habe ich ganz ohne irgendwelche Änderungen am Haupt-Programm eine
minimale Ausgabe produziert. Und das sowohl in der Debug- als auch der
Release-Version.

Wäre schon ganz gut, wenn es nicht noch eine weitere Verbesserung gäbe ...


## Bessere Typen

