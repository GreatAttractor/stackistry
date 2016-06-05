# **Stackistry**

Copyright (C) 2016 Filip Szczerek (ga.software@yahoo.com)

wersja 0.0.3 (2016-05-22)

*Niniejszy program ABSOLUTNIE nie jest objęty JAKĄKOLWIEK GWARANCJĄ. Jest to wolne oprogramowanie na licencji GNU GPL w wersji 3 (lub dowolnej późniejszej) i można je swobodnie rozpowszechniać pod pewnymi warunkami: zob. pełny tekst licencji w pliku LICENSE.*

----------------------------------------

- 1\. Wprowadzenie
- 2\. Obsługiwane formaty plików
- 3\. Interfejs użytkownika
  - 3\.1\. Wybór klatek
  - 3\.2\. Ustawienia przetwarzania
  - 3\.3\. Stabilizacja obrazu
  - 3\.4\. Wizualizacja
- 4\. Pobieranie
- 5\. Budowanie ze źródeł
  - 5\.1\. Budowanie w systemie Linux (i podobnych)
  - 5\.2\. Budowanie pod MS Windows
- 6\. Historia zmian


----------------------------------------
## 1. Wprowadzenie

**Stackistry** realizuje metodę *lucky imaging* przetwarzania obrazów astronomicznych: tworzenie wysokiej jakości zdjęcia z wielu (nawet tysięcy) klatek niskiej jakości (rozmytych, zdeformowanych, z dużą ilością szumu). Wynikowy *stack* wymaga zwykle dodatkowej obróbki, w tym wyostrzania (np. dekonwolucją); kroków tych Stackistry nie wykonuje.

By uruchomić program po pobraniu dystrybucji binarnej dla MS Windows, należy użyć skryptu ``stackistry.bat`` (można utworzyć do niego skrót). Uwaga: obecnie w niektórych wersjach systemu Windows (np. 8.1, ale już nie Vista czy Server 2012) przy uruchamianiu pojawia się komunikat błędu „Program Gio przestał działać” (lub podobny); nie wpływa to na dalszą pracę programu.


----------------------------------------
## 2. Obsługiwane formaty plików

Formaty wejściowe:

- AVI: nieskompresowany DIB (mono lub RGB)
- SER: mono, RGB, „raw color”
- BMP: nieskompresowany 8-, 24- i 32-bitowy
- TIFF: 8 i 16 bitów na kanał nieskompresowany mono lub RGB

Formaty wyjściowe:

- BMP: nieskompresowany 8- i 24-bitowy
- TIFF: 16-bitowy nieskompresowany mono lub RGB

Obsługiwane są pliki AVI do 2 GiB. 64-bitowe wersje Stackistry nie mają ograniczeń co do rozmiaru pozostałych formatów (poza ilością dostępnej pamięci operacyjnej). Na życzenie użytkownika materiał mono może być traktowany jak „raw color” (Stackistry przeprowadzi demozaikowanie).


----------------------------------------
## 3. Interfejs użytkownika

Większość pozycji menu ma odpowiedniki wśród przycisków na pasku narzędziowym lub w menu kontekstowym (otwieranym przez kliknięcie prawym przyciskiem myszy na liście zadań).

Za pomocą standardowych kombinacji klawiszy (``Ctrl-klik``, ``Shift-klik``, ``Ctrl-A``, ``Shift-End`` itd.) zaznaczyć można na liście wiele zadań naraz. Poniższe polecenia wykonywane są dla wszystkich zaznaczonych zadań:

- ``Edycja/Ustawienia przetwarzania...``
- ``EdycjaUsuń z listy``
- ``Przetwarzanie/Rozpocznij przetwarzanie``

W trakcie przetwarzania usuwanie i dodawanie zadań nie jest możliwe.


----------------------------------------
### 3.1. Wybór klatek

Użytkownik może wyłączyć niektóre klatki z przetwarzania używając polecenia ``Edycja/Wybierz klatki...``; jest ono aktywne, gdy zaznaczono tylko jedno zadanie. W oknie dialogowym wyboru klatek można zaznaczyć ich wiele naraz za pomocą standardowych kombinacji klawiszy (``Ctrl-klik``, ``Shift-klik``, ``Ctrl-A``, ``Shift-End`` itd.). Naciśnięcie spacji przełącza stan wybranych klatek; klawisz ``Del`` deaktywuje je.


----------------------------------------
### 3.2. Ustawienia przetwarzania

Ustawienia przetwarzania można zmieniać dla wielu zadań naraz po zaznaczeniu ich na liście i wybraniu ``Edycja/Ustawienia przetwarzania...``. Nazwy zadań, których ustawienia są edytowane, wymienione są w liście w górnej części okna dialogowego ``Ustawienia przetwarzania``.

- ``Rozmieszczenie punktów kotwiczących stabilizacji wideo``

Rozmieszczenie automatyczne powinno sprawdzić się w większości przypadków. W przypadku wybrania trybu ręcznego, okno dialogowe wyboru punktów zostanie wyświetlone w momencie rozpoczęcia przetwarzania danego zadania.

- ``Rozmieszczenie punktów odniesienia``

Obecnie rozmieszczenie automatyczne działa najlepiej dla wideo przedstawiających powierzchnię Słońca lub Księżyca. Dla wideo protuberancji z prześwietloną tarczą i wideo planetarnych niektóre punkty mogą zostać dodane w nieoptymalnych miejscach (gdzie ich śledzenie będzie zawodne). W takim przypadku należy wybrać rozmieszczenie ręczne; okno dialogowe wyboru punktów zostanie wyświetlone dla danego zadania w odpowiednim momencie w trakcie przetwarzania (tj. po zakończeniu fazy szacowania jakości). Należy unikać umieszczania punktów w obszarach z małą ilością szczegółów; dobrze sprawdzą się plamy słoneczne, filamenty, splątane fragmenty protuberancji, kratery, pasy chmur itp.

- ``Kryterium *stackowania*``

Kryterium to dotyczy zarówno śledzenia punktów odniesienia, jak i fazy *stackowania* (końcowego sumowania fragmentów klatek):

Uwaga: zbytnie zwiększanie odsetka (lub liczby) akceptowanych fragmentów klatek wydłuża czas pracy i może pogorszyć jakoś wynikowego *stacku* (jako że zsumowanych będzie więcej fragmentów słabej jakości).

- ``Automatyczny zapis *stacku*``

Jeśli automatyczne zapisywanie nie jest wyłączone, plik wynikowy otrzyma nazwę taką samą jak wideo wejściowe z przyrostkiem ``_stacked``. W przypadku sekwencji plików graficznych nazwą będzie ``stack``.

Niezależnie od wybranego trybu, w każdej chwili można zapisać *stack* dla każdego ukończonego zadania za pomocą polecenia ``Plik/Zapisz stack...``.

- ``Użyj *flatu*``

*Flat* (*flat-field*) to obraz stosowany w celu kompensacji różnic jasności mających źródło w torze optycznym (np. winietowanie, *sweet spot* etalonu, pierścienie Newtona itp.). Stackistry może wykorzystać *flat* stworzony w innym programie (o ile jest w jednym z obsługiwanych formatów), może też wygenerować go samodzielnie, zob. polecenie ``Plik/Wygeneruj flat z wideo...``. Wideo użyte do tego celu musi być niewyostrzone i przedstawiać jednolicie oświetlony widok (bez szczegółów). Bezwględny poziom jasności nie jest istotny, natomiast krzywa tonalna musi mieć tę samą charakterystykę, co w przetwarzanym materiale (np. ustawienie krzywej gamma, o ile obecne w kamerze, musi być identyczne; wartości migawki i wzmocnienia mogą się różnić). W celu osiągnięcia najlepszych efektów należy użyć wideo o długości co najmniej kilkudziesięciu klatek, by szum został uśredniony.

- ``Traktuj materiał mono jako „raw color”``

Po włączeniu tej funkcji Stackisty dokonuje demozaikowania materiału mono metodą interpolacji liniowej wysokiej jakości (Malar, He, Cutler). Układ filtrów kolorowych (RGGB, GRBG itp.) nie jest wykrywany automatycznie, użytkownik musi wybrać właściwy. Jeśli układ nie jest znany, można zrobić to metodą prób i błędów, sprawdzając efekt np. w oknie wyboru klatek; z niewłaściwym układem uzyskamy obraz z pikselami „w kratkę” lub z zamienionymi kanałami czerwonym i niebieskim.

Funkcję można pozostawić wyłączoną dla plików wideo SER w formacie „raw color” (są demozaikowane automatycznie). Może się jednak zdarzyć, że układ filtrów wskazany w SER jest niewłaściwy; włączenie funkcji spowoduje wówczas wymuszenie właściwego układu.


----------------------------------------
### 3.3. Stabilizacja obrazu

Ważną cechą Stackistry jest zdolność do nienadzorowanego przetwarzania dużej liczby zadań, dlatego też automatyczne rozmieszczanie punktów kotwiczących stabilizacji działa zwykle zadowalająco. Jeśli jednak zawiedzie, należy użyć polecenia ``Edycja/Ustaw punkty kotwiczące stabilizacji wideo...`` (lub wybrać ręczne rozmieszczanie punktów kotwiczących w ustawieniach przetwarzania, zob. punkt 3.2). Podobnie jak w przypadku punktów odniesienia, punkty kotwiczące nie powinny znajdować się w obszarach o małej szczegółowości.

Można dodać więcej niż jeden punkt, np. jeśli użytkownik wie, że niektóre z nich przemieszczą się poza kadr wskutek dryfu obrazu (wówczas Stackistry przełączy się na inny z dostępnych punktów). Nawet jeśli stanie się tak ze wszystkimi punktami, Stackistry automatycznie doda nowe w miarę potrzeby.

Jeśli nie występuje znaczny dryf obrazu, wystarczy zdefiniować jeden punkt kotwiczący.


----------------------------------------
### 3.4. Wizualizacja

Włączana/wyłączana poleceniem ``Przetwarzanie/Pokaż wizualizację`` (można użyć go w każdej chwili), funkcja ta generuje „podgląd diagnostyczny” kolejnych etapów przetwarzania. Można użyć jej np. w celu zweryfikowania, że punkty kotwiczące działają prawidłowo, lub że punkty odniesienia są prawidłowo rozmieszczone i śledzone. Uwaga: włączona wizualizacja spowalnia pracę programu.


----------------------------------------
## 4. Pobieranie

Kod źródłowy i pliki wykonywalne dla MS Windows można pobrać pod adresem:
    
    https://github.com/GreatAttractor/stackistry/releases

    
----------------------------------------
## 5. Budowanie ze źródeł

Budowanie ze źródeł wymaga narzędzi do kompilacji C++ (z obsługą C++11) oraz bibliotek gtkmm 3.0 i *libskry*. Wersje (tagi) Stackistry i *libskry* powinny być te same; można też użyć najnowszych rewizji obydwu projektów (aczkolwiek mogą być niestabilne).


----------------------------------------
### 5.1. Budowanie w systemie Linux (i podobnych)

Z kodem dostarczony jest plik ``Makefile`` zgodny z GNU Make. By zbudować aplikację, należy zainstalować biblioteki gtkmm 3.0 (których pakiety noszą zwykle nazwy ``gtkmm30`` i ``gtkmm30-devel`` lub podobne), pobrać (z ``https://github.com/GreatAttractor/libskry/releases``) i zbudować *libskry*, przejść do folderu z kodem źródłowym Stackistry, ustawić odpowiednio wartości w sekcji ``User-configurable variables`` w ``Makefile`` i wykonać:

```
$ make
```

Utworzony zostanie plik wykonywalny ``./bin/stackistry``. Można przenieść go w dowolne miejsce, o ile foldery ``./icons`` i ``./lang`` zostaną umieszczone na tym samym poziomie, co ``./bin``.


----------------------------------------
## 5.2. Budowanie pod MS Windows

Budowanie z użyciem dostarczonego pliku ``Makefile`` zostało przetestowane w środowisku MinGW/MSYS. Plik wykonywalny (``stackistry.exe``) tworzony jest po wykonaniu tych samych kroków, co w punkcie **5.1** (w linii poleceń środowiska MSYS).


**Uwagi dodatkowe**

Instalator MSYS2 pobrać można z ``https://msys2.github.io/`` (należy postępować zgodnie z zamieszczoną tam instrukcją). Po zakończeniu instalacji trzeba uruchomić konsolę MSYS i zainstalować wymagane narzędzia i biblioteki. Dla 64-bitowego GCC należy wykonać:

```
$ pacman -S make base-devel mingw-w64-x86_64-toolchain
```

(pakiet 32-bitowy nosi nazwę ``mingw-w64-i686-toolchain``).

By zainstalować 64-bitowe GTK i gtkmm, wykonać:

```
$ pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gtkmm3
```

W przyszłości nazwy pakietów mogą ulec zmianie; by wyświetlić prawidłowe nazwy dla konkretnych bibliotek, można wykonać:

```
$ pacman -Ss gtk3
$ pacman -Ss gtkmm
```

Przed budowaniem należy odpowiednio ustawić zmienną PATH. Dla 64-bitowego GCC:

```
export PATH=/mingw64/bin:$PATH
```

MSYS montuje napędy jako `/<litera_dysku>`, więc jeśli źródła Stackistry zostały umieszczone w:

```
C:\Users\NazwaUżytkownika\Documents\stackistry
```

będą widoczne z poziomu MSYS jako:

```
/c/Users/NazwaUżytkownika/Documents/stackistry
```

Po zbudowaniu, Stackistry można uruchomić z linii poleceń MSYS (``./bin/stackistry.exe``). By możliwe było uruchomienie programu bezpośrednio z Eksploratora Windows, niezbędne pliki DLL muszą być umieszczone w odpowiednich (względem ``stackistry.exe``) miejscach (zob. binarną dystrybucję Stackistry dla porównania).


----------------------------------------
## 6. Historia zmian

```
0.0.3 (2016-05-22)
  Nowe funkcje:
    - Demozaikowanie materiału „raw color”
  Poprawki błędów:
    - Błąd przy otwieraniu plików SER nagranych narzędziem Genika
    - Zła kolejność kanałów RGB przy zapisywaniu BMP

0.0.2 (2016-05-08)
  Poprawki błędów:
    - Naprawiono błędy podczas przetwarzania serii plików TIFF
    - Wykorzystywane są wszystkie fragmenty, jeśli kryterium to
      „liczba najlepszych”, a próg jest większy niż liczba aktywnych klatek

0.0.1 (2016-05-01)
  Pierwsza wersja.
```
