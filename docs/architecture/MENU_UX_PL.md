# Menu i UX wyświetlacza MY23

Zmiany w menu zachowują katalog 46 stron benzynowych i 55 diesla. Nawigacja,
preferencje i przesyłanie tekstu mają osobne moduły. Nowe etykiety używają ASCII,
aby nie zależeć od obsługi polskich znaków przez licznik. Dotychczasowe skróty
parametrów i etykiety opcji funkcji pozostają w swoich szablonach.

## Obsługa

Menu korzysta z przycisków tempomatu, gdy CC i ACC są wyłączone. Po ich
wyłączeniu trzeba najpierw zwolnić przyciski. Przytrzymanie RES przez 800 ms
otwiera menu na ostatnim ulubionym parametrze. Przycisk dystansu ma tę samą
funkcję co RES.

| Gest | Działanie |
| --- | --- |
| Krótkie RES, następnie zwolnienie | Wejście lub wybór; na ekranie wartości przejście do menu głównego |
| RES przez 800 ms | Powrót o poziom; w menu głównym zapis i zamknięcie |
| Delikatne wychylenie w dół / w górę | Następna / poprzednia pozycja, jeden krok na naciśnięcie |
| Mocniejsze wychylenie | Następna / poprzednia grupa parametrów, funkcji lub opcji funkcji |

W ulubionych mocniejsze wychylenie również zmienia pojedynczą pozycję.
Przejście przez delikatne wychylenie do mocniejszego może najpierw wykonać
pojedynczy krok, a potem skok grupy. Przytrzymanie nie uruchamia autorepeat.
Przerwa w ramkach przycisków powyżej 300 ms wymaga ponownego zwolnienia.
Zwolnienie RES po długim przytrzymaniu nie wykonuje dodatkowego wyboru.

Menu główne: **Ulubione → Parametry → Funkcje → Ustawienia → Informacje**.
Parametry mają grupy: Wszystkie, Silnik, Temperatury, Zasilanie, DPF / AdBlue,
Osiągi, Pozostałe. Pusta grupa wyświetla `Brak stron`, a puste ulubione
`Brak ulubionych`; powrót działa również z pustej listy.

## Personalizacja

1. Otwórz `Ustawienia → Wybierz ulubione`. RES dodaje lub usuwa wybraną stronę;
   znak `+` oznacza wybraną. Limit wynosi sześć stron na profil silnika.
2. W `Kolejnosc ulub.` wybierz pozycję i naciśnij RES. Gwiazdka oznacza tryb
   przenoszenia. Wychylenia przesuwają pozycję, kolejne RES kończy przenoszenie.
   Przesuwanie zatrzymuje się na krańcach listy.
3. W `Widoczne strony` RES zmienia widoczność w zwykłym katalogu. Ukrycie strony
   nie usuwa jej z ulubionych. Automatyczny ekran wyniku może pokazać ukrytą
   stronę bez trwałego zmieniania tej preferencji.
4. W `Sortowanie` RES przełącza kolejność grupową i A–Z według krótkiej nazwy
   strony. Sortowanie dotyczy katalogu i edytorów; własna kolejność ulubionych
   pozostaje zachowana. Mocniejsze wychylenie w edytorach zmienia grupę.
5. Użyj `Zapisz` lub wróć długim RES. Wyjście z edytora zapisuje preferencje,
   z opcji funkcji zapisuje również ustawienia. Zamknięcie z menu głównego
   zapisuje oba rekordy. Zapamiętywane są ostatnie strony grup i ulubionych.

Ulubione, widoczność i ostatnie strony są oddzielne dla benzyny i diesla;
wybór sortowania jest wspólny. Zmiana profilu czyści cache pomiarów.
Komunikat `Blad zapisu. RES` oznacza nieudany zapis. Menu pozostaje otwarte,
zmiany pozostają w RAM. RES ponawia zapis; po błędzie przy powrocie ponawia
powrót, zamiast przypadkowo przełączać opcję. Przy trwałej awarii zapisu
zamknięcie z menu głównego również pozostaje zablokowane. Rekord ustawień
i rekord preferencji są zapisywane osobno, bez wspólnej transakcji.

Funkcje zmieniające stan wymagają drugiego RES w ciągu 3 sekund. Zmiana
pozycji lub powrót anuluje potwierdzenie. Pozostają warunki dostępności,
postoju i trybu Dyno istniejących funkcji. `Zlecono` oznacza przyjęcie żądania,
nie potwierdzenie wykonania przez ECU. Pozycja Immobilizer pokazuje stan;
dotychczasowy osobny gest zmiany jego stanu pozostaje w sterowaniu kierownicy.
Niedokończony odczyt DTC, demonstracyjne logowanie i pozostałe puste pozycje
nie są prezentowane jako gotowe funkcje menu.

## Co wpływa na płynność

- Zdarzenia przycisków zależą od czasu i przejść stanu, a nie od liczby
  odebranych ramek. Nie ma opóźnień blokujących w kontrolerze menu.
- Menu formatuje bieżący widok; listę sortuje przy jej budowaniu, nie w każdej
  iteracji. Model i bufory mają stały rozmiar, bez alokacji sterty.
- Renderowanie okresowe ma interwał 100 ms. Identyczny tekst jest pomijany
  przez 500 ms, po czym wysyłany ponownie jako podtrzymanie wyświetlania.
- UART przechowuje tylko najnowszy oczekujący ekran. Polecenia zachowują FIFO,
  aktywny bufor transmisji nie jest nadpisywany. Ekran może wyprzedzić oczekujący
  odczyt statusu najwyżej raz, więc ciągłe przewijanie nie blokuje statusów.
- BH kończy wysyłanie fragmentów aktywnego tekstu przed rozpoczęciem następnego.
  Nieudane przyjęcie ramki przez kolejkę CAN nie gubi fragmentu. Fabryczna
  ramka tekstu nie restartuje trwającej transmisji BACCAble.
- Parametry z natywnych ramek CAN trafiają do cache po odebraniu właściwej
  ramki o wymaganej długości. UDS odpytuje wybraną stronę najwyżej co 500 ms
  i dopiero po 150 ms od zmiany strony. Kasowanie błędów wstrzymuje te odczyty.
  Odpowiedzi są powiązane z ECU, DID, profilem i aktualną stroną.
- Pomiar bez aktualizacji przez ponad 3 s pokazuje `--`. Lokalne rekordy osiągów
  i odczyt wolnej pamięci nie podlegają temu terminowi. Liczby mają stałą
  szerokość i zachowują część dziesiętną; przekroczenie pola pokazuje `--`.

100 ms jest okresem renderowania, nie zmierzonym czasem reakcji licznika.
Nadal obowiązuje odstęp UART ponad 250 ms i przesyłanie tekstu po trzy znaki
co najmniej co 50 ms: sześć fragmentów dla 18 znaków, osiem dla 24.
Przy szybkim przewijaniu urządzenie może pomijać pośrednie oczekujące ekrany.
Ostateczna płynność, zachowanie fabrycznych komunikatów i Race mask wymagają
próby w samochodzie. Zmiany nie zwiększają mocy ani osiągów pojazdu.

## Zgodność i rozszerzanie

`LARGE_DISPLAY` wybiera 24 znaki; bez tej flagi jest 18. C1, C2 i BH muszą
mieć zgodną szerokość, ponieważ wpływa ona również na długość ramki UART.
Ustawienie `IPC_MY23_IS_INSTALLED` nie zastępuje flagi `LARGE_DISPLAY`.
Informacje pokazują wersję C1, odpowiedzi C2/BH i ustawienie MY23 z szerokością.
Brak odpowiedzi przez ponad 5 sekund daje `brak odpowiedzi`; starsze firmware
bez nowej odpowiedzi statusowej również może tak się przedstawiać.

Preferencje zajmują jawnie serializowany rekord 80 bajtów typu `0x104`, wersja 1,
w istniejącym slocie widoczności. Jeśli nie ma nowego rekordu, menu importuje
widoczność poprzedniego rekordu `0x103` według starych indeksów. Zapis zastępuje
ten rekord nowym formatem. Ustawienia, statystyki i pozycje lusterek mają nadal
swoje odrębne rekordy. To migracja z poprzedniej wersji tej gałęzi, a nie
dekoder dowolnego formatu oryginalnego firmware 3.1.1.

Dodawanie strony:

1. Dodaj lub wykorzystaj `ParameterDefinition` w `diagnostics/parameter_catalog.c`.
   Dla nowego parametru natywnego dodaj odczyt w `native_parameters.c` oraz
   aktualizację cache po jego rzeczywistej ramce w `parameter_cache.c`.
2. Dodaj `ParameterPage` z `id`, `group`, krótką `label`, szablonem `name`
   i dwoma `parameter_ids`. Identyfikatory stron są trwałe: benzyna `0x01..0x40`,
   diesel `0x81..0xc0`. Wybierz nieużywany identyfikator; nigdy nie numeruj
   istniejących stron ponownie ani nie wykorzystuj usuniętego ID do innego celu.
3. Zaktualizuj licznik stron profilu. Katalog/lista mieści do 60 stron na profil;
   rozszerzenie ponad limit wymaga zmiany tablic i testów. Grupa 0 oznacza
   widok Wszystkie; rzeczywistym stronom przypisz grupę 1–6.
4. Nową akcję dodaj do enum i tabeli `actions` w `features/menu.c`, określ
   dostępność, warunki wykonania, polecenie i sposób pokazywania stanu.
   Logikę działania urządzenia umieść we właściwym module funkcji.
5. Uruchom testy, lint i kompilację właściwych wariantów. Kontroluj rozmiar:
   program C1 mieści się w 64 KiB, ale ma mały zapas Flash. Dodawanie tekstów
   i funkcji wymaga każdorazowej kontroli wyniku linkera.

## Weryfikacja lokalna

Weryfikacja 2026-09-10, `VERSION=menu-ux`: sześć programów testowych przeszło
z ASan/UBSan, cppcheck przeszedł dla C1/C2/BH/CAN. Przeszły wszystkie cztery
kompilacje bazowe i dodatkowo C1/C2/BH dla 24 znaków. Rozszerzony wariant C1
obejmował również benzynę, MY23 i sterownik LED.

| Wariant | Flash programu: text + data | Statyczny RAM: data + bss |
| --- | ---: | ---: |
| C1, 18 znaków | 65 048 B | 10 668 B |
| C1, 24 znaki + benzyna + MY23 + LED | 65 080 B | 11 108 B |
| C2, 18 znaków | 22 216 B | 10 728 B |
| BH, 18 znaków | 23 212 B | 10 708 B |
| CAN | 21 428 B | 7 104 B |

Zapas w 64 KiB obszaru programu C1 wynosi odpowiednio 488 i 456 bajtów.
Statyczny RAM nie uwzględnia stosu w czasie pracy. Inna wersja kompilatora,
flagi lub tekst wersji mogą zmienić te wyniki. Tej wersji nie wgrano na
urządzenie ani nie zweryfikowano w zdalnym CI.

Testy wykonują kod produkcyjny z atrapami HAL i pamięci. `make -C tests test`
uruchamia sześć programów z ASan/UBSan, w tym menu dla 18 i 24 znaków oraz
rzeczywisty sterownik UART. Sprawdzane są gesty, utrata ramek, przepełnienie
zegara, pełne transmisje tekstu, ponawianie HAL_BUSY, kolejność poleceń,
sortowanie, ulubione, migracja, pusty katalog, pamięć pozycji, błąd zapisu,
zmiana silnika, spóźniona odpowiedź UDS i wygasanie danych.

Polecenia bazowe:

```sh
make -C tests test
make -C firmware/baccable FLAVOR=C1 lint
make -C firmware/baccable -j4 FLAVOR=C1 VERSION=menu-ux
make -C firmware/baccable -j4 FLAVOR=C1 BUILD_DIR=build/C1-menu-large \
  VERSION=menu-ux EXTRA_CPPFLAGS="-DLARGE_DISPLAY -DIPC_MY23_IS_INSTALLED -DIS_GASOLINE -DLED_STRIP_CONTROLLER_ENABLED"
```

Powtórz bazową kompilację i lint dla C2, BH i CAN. Dla zestawu 24-znakowego
skompiluj również C2 i BH z `-DLARGE_DISPLAY -DIPC_MY23_IS_INSTALLED`.
Lokalnie użyto ARM GNU Toolchain 15.2.Rel1; ustaw `TOOLCHAIN` na jego prefiks,
jeśli nie znajduje się w PATH. Wyniki sprzętowe ani zdalnego CI dla tych zmian
nie są zastępowane wynikami testów hosta.
