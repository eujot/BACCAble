# BACCAble — instrukcja obsługi w samochodzie

Ta instrukcja opisuje menu na zestawie wskaźników i funkcje w bieżącym kodzie
firmware. Uwzględnia poprawki niezawodności startowego trybu jazdy i zawieszenia
oraz funkcję Rain lights po v5-beta-29. Startowy tryb Race i łagodniejsze
ustawienie zawieszenia zostały potwierdzone w samochodzie 24 października 2026;
potwierdzenie działania świateł z funkcji Rain lights nadal jest otwarte.
Instrukcja dotyczy urządzenia już zamontowanego
i podłączonego do samochodu. Nie obejmuje montażu, okablowania ani wgrywania
oprogramowania. Kontrolery C1, C2 i BH powinny mieć pasujący do siebie zestaw
oprogramowania z tego samego wydania.

To polskie tłumaczenie [aktualnej instrukcji angielskiej](BACCAble_USER_GUIDE_EN.md).
Nazwy opcji, odczytów i komunikatów pozostawiono po angielsku, tak jak na
wyświetlaczu. Ich znaczenie i sposób użycia opisano po polsku.

Podstawą opisów funkcji była [starsza instrukcja angielska](Baccable_manual_EN.pdf).
Ścieżki menu, sterowanie, nazwy i szczegóły działania sprawdzono w aktualnym kodzie.
Jeśli starsza instrukcja podaje coś innego, stosuj opis z tego dokumentu.
Dostępność odczytów i funkcji zależy od wyposażenia samochodu oraz oprogramowania
jego sterowników. Sama obecność opcji w menu nie oznacza, że samochód ją obsługuje.

## Spis treści

- [Pierwsze użycie](#pierwsze-użycie)
- [Przyciski i nawigacja](#przyciski-i-nawigacja)
- [Układ menu](#układ-menu)
- [Symbole i komunikaty](#symbole-i-komunikaty)
- [Tymczasowe ustawienia odświeżania IPC](#tymczasowe-ustawienia-odświeżania-ipc)
- [Ulubione i układ odczytów](#ulubione-i-układ-odczytów)
- [Ustawienia: Features](#ustawienia-features)
- [Korzystanie z funkcji](#korzystanie-z-funkcji)
- [Akcje: Actions](#akcje-actions)
- [Informacje: Information](#informacje-information)
- [Jak rozumieć odczyty](#jak-rozumieć-odczyty)
- [Odczyty dla silników benzynowych](#odczyty-dla-silników-benzynowych)
- [Odczyty dla silnika Diesla](#odczyty-dla-silnika-diesla)
- [Przechwytywanie CAN i diagnostyka przez USB](#przechwytywanie-can-i-diagnostyka-przez-usb)
- [Rozwiązywanie problemów](#rozwiązywanie-problemów)
- [Różnice względem starej instrukcji](#różnice-względem-starej-instrukcji)
- [Źródła i zakres sprawdzenia](#źródła-i-zakres-sprawdzenia)

## Pierwsze użycie

1. Zatrzymaj samochód i włącz zapłon. Uruchom silnik, jeśli wymaga tego funkcja
   lub odczyt. Część danych jest dostępna tylko przy pracującym silniku.
2. Wyłącz **CC i ACC**, a następnie puść przyciski kierownicy.
3. Przytrzymaj **RES przez około 0,9 sekundy** i puść. Otworzą się Favorites.
4. **Naciśnij RES dwukrotnie**, aby wrócić do menu głównego. Przyciskami góra/dół
   wybierz `Settings`, naciśnij RES i otwórz `Features`.
5. W `Engine` wybierz `2.0 I4`, `2.9 V6` lub `2.2 D` krótkimi naciśnięciami albo
   góra/dół. Przytrzymaj RES, aby zapisać bezpośrednio, lub wykonaj dwuklik,
   aby wyświetlić pytanie o zapis.
   Ustawienie pozostanie na ekranie; osobny dwuklik wróci do listy Features.
6. Sprawdź `MY23 display`. Profil MY23 korzysta z dwóch linii niezależnie od opcji
   kompilacji 18/24 znaków dla starszego ekranu. Wgraj zgodny zestaw C1/C2/BH;
   ta poprawka wprowadza dłuższy pakiet przesyłania ekranu.

C1 obsługuje CAN układu napędowego i przyciski menu, C2 CAN podwozia, a BH CAN
nadwozia i wysyłanie tekstu do IPC. Wersje sprawdzisz w `Information`.
Ustawiaj menu na postoju. Przestrzegaj
[ograniczeń użytkowania projektu](../README.md#disclaimer).

## Przyciski i nawigacja

Przy wyłączonych CC i ACC przycisk odstępu ACC działa w menu tak samo jak RES.

| Naciśnięcie | Działanie |
| --- | --- |
| Przytrzymanie RES przez około 0,9 s przy zamkniętym menu | Otwiera Favorites. |
| Krótkie RES i puszczenie | Otwiera lub używa pozycji albo zmienia szkic ustawienia. Strona odczytu nie reaguje. |
| Dwuklik RES | Powrót. Rozpocznij drugie naciśnięcie do 0,28 s po pierwszym puszczeniu. |
| Przytrzymanie RES przez około 0,9 s w edytorze | Zapisuje widoczny szkic bezpośrednio, również po pytaniu o zapis. |
| Przytrzymanie RES przy widocznym pytaniu APPLY/STORE | Zatwierdza akcję lub zapis pozycji lusterek. |
| Lekkie dół/góra | Następna/poprzednia pozycja albo zmiana szkicu. Listy zawijają się. |
| Przytrzymanie lekkiego dół/góra | Powtarza po 0,5 s, następnie co 0,18 s. Bez powtarzania w Actions ani przy przesuwaniu ulubionej strony. |
| Mocniejsze dół/góra | Zmienia grupę; podczas edycji zmienia wartość szkicu. |

Pojedyncze naciśnięcie jest wykonywane po zakończeniu okna dwukliku, więc ma
krótkie celowe opóźnienie. Naciśnięcia krótsze niż 30 ms są pomijane. Trzecie
szybkie naciśnięcie po dwukliku nie wykonuje dodatkowej akcji; celowe
przytrzymanie rozpoczęte zaraz po dwukliku nadal działa. Przewijanie anuluje
oczekującą parę naciśnięć. Po przerwie ponad 0,3 s w komunikatach o przyciskach
puść przycisk i spróbuj ponownie. Puszczenie po przytrzymaniu nie wybiera pozycji.
Mocniejsze naciśnięcie może najpierw wykonać zwykły krok, potem zmianę grupy.

### Powrót, bezczynność i zapisywanie

Wejście do edytora pokazuje zapisaną wartość i samo jej nie zmienia. Przytrzymanie
RES w edytorze zapisuje bezpośrednio — wcześniejszy dwuklik nie jest wymagany.
Dwuklik cofa o jeden poziom. Bez zmian wraca od razu. Po zmianie szkicu pierwszy
dwuklik pokazuje `HOLD SAVE•2X DISCARD` na MY23 albo `Hold save/2x drop` na
starszym ekranie. Przytrzymanie zapisuje i stosuje zmianę; ustawienie zostaje
na ekranie z komunikatem `SAVED` przez około trzy sekundy. Osobny dwuklik
wraca. Dwuklik zamiast
przytrzymania w ekranie potwierdzenia odrzuca szkic i wraca, zachowując oryginał.
Błąd zapisu pozostawia szkic i możliwość kolejnej próby przytrzymaniem.
Po restarcie wczytywane są tylko zapisane wartości. Tryb USB zmienia się dopiero
po udanym jawnym zapisie. Sam wybór CAN lub wyjście z niezapisanego szkicu
nie włącza tego trybu.

Tak samo edytujesz Features, sloty ulubionych, Enabled lusterek i Sort order.
Nie ma Save & Exit. Bieżące odczyty pozostają widoczne. Pozostałe widoki wracają
do Favorites po 30 s bezczynności, a edytory po 60 s. Niedokończone szkice są
odrzucane. Aktywny odczyt lub kasowanie błędów opóźnia automatyczny powrót.

Edytory `Page favorites`, `Favorite order` i `Shown pages` mają takie same
szkice, potwierdzenie, zapis przytrzymaniem i odrzucanie. Zapis zostawia Cię
w edytorze; osobny dwuklik wraca do Settings. Bezczynność odrzuca niezapisane
zmiany. Zapamiętane strony odczytów są zapisywane przy wyjściu ze zwykłych widoków. Przy `× Save failed: RES` krótkie RES lub przytrzymanie ponawia
zapis i wyjście, a dwuklik anuluje próbę wyjścia i zostawia bieżący widok.
Samo czekanie nie ponawia zapisu.

## Układ menu

```text
Menu główne
├─ Favorites                 Wybrane przez Ciebie bieżące odczyty
├─ Readings                  Wszystkie dostępne odczyty, według grup
│  ├─ All readings           Wszystkie odczyty
│  ├─ Engine                 Silnik
│  ├─ Temperatures           Temperatury
│  ├─ Battery                Akumulator
│  ├─ DPF / AdBlue           Filtr DPF i AdBlue
│  ├─ Performance            Osiągi
│  └─ Other                  Pozostałe
├─ Actions                   Uruchamianie funkcji i operacji diagnostycznych
├─ Settings                  Ustawienia
│  ├─ Features               Ustawienia urządzenia i funkcji samochodu
│  │  └─ Park mirror         Lusterka podczas cofania
│  │     ├─ Enabled          Włączenie funkcji
│  │     ├─ Store position   Zapis pozycji
│  │     └─ < Back           Powrót
│  ├─ Favorites              Favorite 1–6 → Slot 1–2 → strona odczytów
│  ├─ Shown pages            Pokazywanie i ukrywanie stron w Readings
│  ├─ Sort order             Sortowanie według grup albo alfabetycznie
│  └─ BACCAble IPC           Tymczasowe próby odświeżania
└─ Information               Wersje, stan przycisków i Demo
```

Ten układ Settings dotyczy MY23 oraz każdego profilu po zapisaniu zestawu
dwu slotów ulubionych. Przed pierwszym takim zapisem starszy profil zachowuje
kolejność: Features, Page favorites, Shown pages, Favorite order, Sort order,
Favorites, BACCAble IPC.

Kolejność menu głównego jest stała. Dostępność stron zależy od wybranego silnika
oraz ustawień `Advanced pages` i `Shown pages`. Grupa bez dostępnych stron może
pokazać `No pages`. Pełne wykazy w dalszej części obejmują również strony
domyślnie ukryte.

## Symbole i komunikaty

Starszy profil, zależnie od kompilacji, wykorzystuje 18 lub 24
pozycje tekstowe. Nazwa na liście może różnić się od podpisu bieżącego odczytu,
aby zrobić miejsce na wartości. Na przykład strona `Batt charge BCM` wyświetla
odczyt podpisany `Batt BCM SOC`.

MY23 ma **16 widocznych znaków w pierwszej linii i 22 w drugiej**. Lista
pokazuje bieżącą pozycję z `›`, a pod nią następną. Edytor pokazuje ustawienie
oraz szkic wartości z `HOLD SAVE`. Druga linia zawiera również pytanie o zapis
lub jego wynik. Ulubione pokazują po jednej wybranej stronie odczytów w każdej
linii. Readings pokazuje bieżącą stronę w pierwszej linii, a nazwę następnej
strony w drugiej.
`SAVED` potwierdza zapis ustawienia, a `×` lokalny błąd.
Starszy profil zachowuje pierwszą linię 18/24 znaki i podpis `BACCAble beta`.
Obsługa przycisków i szkiców jest taka sama, a pytania mieszczą się w jednej linii.

BH używa ostatniego zaobserwowanego kompletnego źródła tekstu multimediów,
np. USB lub CarPlay, zamiast wymuszać Bluetooth. Powtarza całe menu mniej więcej
co sekundę i zaczyna ponownie po ustaniu tekstu radia. Zamknięcie przywraca
ostatni kompletny tekst radia. Oba pola MY23 są osobno czyszczone i dopełniane,
więc skrócenie tytułu nie przesuwa drugiej linii ani nie zostawia starych znaków.
Czytelność, wyrównanie i odzyskiwanie ekranu po radiu wymagają próby w samochodzie.

Pierwsze naciśnięcie góra/dół odświeża menu od razu. Przy szybkim naciskaniu
każde naciśnięcie zmienia wybór, lecz ekran może pominąć pośrednie pozycje i po
krótkiej przerwie pokazać ostatnią. Opóźnienie zależy od wybranego tempa IPC
(10, 20 lub 50 ms); komunikaty radia mogą je dodatkowo wydłużyć.

| Symbol lub komunikat | Znaczenie i sposób obsługi |
| --- | --- |
| `O` / `Ø` przed opcją | Wyłączona / włączona albo niewybrana / wybrana na liście. Krótkie RES zmienia wybór. |
| `ON` / `OFF` w informacji o stanie | Stan zgłoszony przez urządzenie lub śledzony przez BACCAble. Nie zawsze można go zmienić na tej stronie. |
| `>` | Wejście do podmenu lub uruchomienie operacji przyciskiem RES. |
| `*` | Trwa edycja liczby albo wybrano ulubioną stronę do przesunięcia. |
| `< Back` | Powrót z podmenu. Dwuklik RES również cofa. |
| `!` | Trzeba spełnić warunek lub zwrócić uwagę na problem, np. `! Start engine` — uruchom silnik, `! Stop the car` — zatrzymaj samochód. |
| `?` | Stan jest nieznany lub brakuje potwierdzenia, np. `? BH no reply` — brak odpowiedzi BH. |
| `×` | Operacja w urządzeniu nie powiodła się, np. zapis ustawień lub wysłanie polecenia. |
| `--` w odczycie | Brak poprawnej wartości: dane nie dotarły, są nieaktualne, nieobsługiwane lub błędne albo liczba nie mieści się w polu. To nie oznacza zera. |
| `WAIT` / `Request queued` | Polecenie oczekuje na obsługę. Nie potwierdza to jego wykonania przez samochód. |
| `Request sent` | Sekwencja wysyłania zakończyła się. Sprawdź rzeczywisty stan samochodu. |
| `No confirmation` / przekroczenie czasu oczekiwania | Potwierdzenie nie nadeszło w przewidzianym czasie. Upływ tego czasu nie anuluje wysłanego polecenia. |
| `RUN` / `MISS` | Pomiar przyspieszenia trwa / nie uzyskano poprawnego wyniku w przewidzianym czasie. |

`°C` oznacza stopnie Celsjusza. Na stronach z kilkoma odczytami może występować
samo `C`. Przy korekcie zapłonu `°` oznacza kąt. Wartości ujemne zachowują znak
minus. Symbole `«`/`»` przy edycji liczby wskazują możliwość regulacji, jeśli
mieści się na nie miejsce. Listy nie pokazują numeru bieżącej pozycji.

## Tymczasowe ustawienia odświeżania IPC

Otwórz `Settings → BACCAble IPC`. Góra/dół wybiera wiersz, a krótkie kliknięcie
RES przełącza wartość i stosuje ją od razu. Podwójne kliknięcie wraca do Settings.
Te opcje próbne nie wymagają HOLD SAVE i nigdy nie są zapisywane w pamięci Flash.

| Wiersz / wybór | Działanie |
| --- | --- |
| `Safe 50ms` | Wolniej: 50 ms odstępu między ramkami CAN menu. |
| `Quick 20ms` | Domyślnie: odstęp 20 ms między ramkami. |
| `Fast 10ms` | Odstęp 10 ms między ramkami. |
| `Write Full` | Domyślnie: wysyła cały ekran, wraz ze spacjami czyszczącymi pola, od pierwszego do ostatniego fragmentu. Treść nie zmienia się w trakcie tej transmisji; oczekuje tylko najnowszy kolejny ekran. |
| `Write Delta` | Wysyła zmienione fragmenty; nowe wartości mogą zastąpić treść w trakcie transmisji. To tryb porównawczy, w którym mogą mieszać się stare i nowe napisy. |
| `Reset default` | Od razu przywraca `Quick 20ms` i `Write Full`. |

Zapamiętuj parę, np. **Quick + Full**. MY23 potrzebuje 13 ramek: pełna transmisja
zajmuje około 650 / 260 / 130 ms dla Safe / Quick / Fast, plus oczekiwanie na
CAN lub radio. Nowy ekran może też czekać na zakończenie trwającej transmisji.
To IPC decyduje, kiedy pokaże otrzymane fragmenty. Full zapobiega mieszaniu
różnych ekranów przez firmware, ale nie gwarantuje jednoczesnej podmiany
wszystkich znaków. W obu trybach pozostaje ponowne wysyłanie całego menu co
sekundę. Nadal działa rozpoznawanie źródła i oczekiwanie po transmisji radia
z ograniczonym czasem odroczenia. Przywracanie tekstu radia zachowuje odstęp 50 ms.

Wgraj pasujące obrazy C1/C2/BH. Te opcje są tylko w RAM, również wtedy, gdy
zapiszesz inne ustawienia Features. Pełny restart przywraca Quick + Full.
Odłącz **zarówno zasilanie USB, jak i OBD**, aby wyłączyć całe urządzenie.
BH dodatkowo wraca do Quick + Full po pięciu sekundach bez podtrzymania z C1.
Działające C1 ponawia wybraną parę co sekundę, również po wyjściu z menu;
restart samego BH może więc spowodować ponowne otrzymanie tej pary.
`UART busy: retry` oznacza, że kliknięcie nie zostało przyjęte — powtórz je.
Przyjęcie do kolejki nie jest potwierdzeniem wykonania przez BH ani IPC.

Porównuj ustawienia przy tym samym źródle audio. Powtarzaj: długie i krótkie
nazwy menu, przewijanie dwóch pozycji, bieżące ulubione, zmiana utworu i powrót
do menu. Zapisuj parę ustawień, nakładanie napisów, opóźnienie i źródło audio.
Zacznij od Quick + Full, potem opcjonalnie Safe + Full lub Fast + Full; opcjonalnie porównaj
Delta. Jeśli działanie się pogorszy, wybierz Reset default lub odłącz całe zasilanie.

## Ulubione i układ odczytów

Ulubione odpytują każdy parametr diagnostyczny raz na cykl, nawet jeśli występuje w obu liniach. Przy ośmiu pomiarach odświeżenie całego zestawu może trwać cztery sekundy. Wartości pozostają widoczne przez ten czas, a po utracie odpowiedzi pojawia się `--`. Odczyty przychodzące bezpośrednio z CAN zachowują dotychczasowy czas ważności.

### Domyślne zestawy ulubionych

Świeża instalacja (benzyna) startuje z sześcioma tematycznymi zestawami, z których
każdy łączy stronę pierwszej linii ze stroną drugiej linii. Wyświetlacz 1-liniowy
pokazuje tylko pierwszą kolumnę, a 2-liniowy obie linie. Po starcie wybrany jest
zestaw **S1 Elektryka**.

| Zestaw | Linia 1 | Linia 2 |
| --- | --- | --- |
| S1 Elektryka | `Oil L / mm / %` | `Batt charge BCM` |
| S2 Osiągi | `Power / torque` | `Boost pressure` |
| S3 Temperatury | `Oil/water/gear C` | `Intercooler out` |
| S4 Jazda | `Speed` | `Gear` |
| S5 Timery | `0-100 km/h` | `Best 0-100` |
| S6 Olej / Multiair | `Oil/water/IC C` | `Misfires/MA temp` |

Diesel zachowuje cztery dotychczasowe domyślne strony i nie ma stron drugiej
linii. Nowe instalacje używają układu ze slotami Favorites na obu profilach IPC.
Pary są ustawiane tylko wtedy, gdy nie ma zapisanych ustawień stron ani slotów.
Zapisane ulubione pozostają zachowane, nawet jeśli pierwsza strona ma taki sam
numer jak strona domyślna. Edycja tych zestawów przebiega według poniższych kroków.

### Dwie strony odczytów w jednym ulubionym widoku

1. Otwórz `Settings → Favorites` i wybierz `Favorite 1` do `Favorite 6`.
2. Wejdź krótkim RES. Wybierz `Slot 1` lub `Slot 2`, naciśnij RES, znajdź stronę
   z katalogu Readings przyciskami góra/dół i zatwierdź krótkim RES. `Empty` czyści slot.
3. **Slot 1 jest główną stroną odczytów**. MY23 pokazuje ją w pierwszej linii,
   a Slot 2 w drugiej. Pusty slot główny ukrywa cały widok.
   Kolejność zmieniasz przez zastępowanie slotów. Strona już obecna w innym
   slocie jest z niego usuwany. Edycja jest dostępna wyłącznie w Settings.
4. Przytrzymanie zapisuje bezpośrednio, również podczas wybierania wartości slotu.
   Możesz też wykonać dwuklik, aby zobaczyć pytanie, a następnie przytrzymać,
   aby zapisać, lub wykonać kolejny dwuklik, aby odrzucić. Udany zapis pozostawia wybrany slot na ekranie;
   osobny dwuklik wraca.
5. Bieżące wartości zobaczysz w `Favorites` menu głównego. Starszy ekran
   pokazuje główną stronę zapisanego zestawu w jednej linii.

Po powrocie z innych list pozostaje wybrany ten sam zestaw ulubionych. Tak samo
działa zmiana filtra silnika, jeśli zestaw nadal jest dostępny. W przeciwnym
razie wyświetla się pierwszy dostępny zestaw. Wybór jest pamiętany osobno dla
benzyny i diesla do ponownego uruchomienia urządzenia.

Każdy slot pokazuje pomiary z wybranej strony katalogu. MY23 usuwa zbędne
odstępy i w razie potrzeby skraca podpis; starszy ekran zachowuje dotychczasowy
format. Na przykład
`Oil/coolant temp` zawiera dwa odczyty w jednej linii i po dodaniu do ulubionych
pozostaje jednym slotem. `--` oznacza brak aktualnej wartości. `Gear -` obejmuje surowy kod
`0xF` („brak informacji”); nie jest to piętnasty bieg.

Na MY23 `Oil/water/IC C` pokazuje cztery temperatury jako `O90 W85 I40 X35`:
olej, płyn chłodzący, wlot i wylot intercoolera. Gdy wartości mają trzy cyfry,
odstępy są ograniczane, np. `O101W102I103X104`, aby zachować wszystkie odczyty
w 16 znakach. `Oil/coolant temp` pokazuje `Oil 101° W 88°`, gdzie `W` oznacza
płyn chłodzący. `Misfires cyl 1-4` pokazuje liczniki w kolejności cylindrów,
np. `12/ 23/ 34/ 45`.

Wyświetlane i edytowane są tylko Slot 1 i Slot 2. Wcześniejsze rekordy pięciu
slotów zachowują w pamięci miejsca 3–5 dla zgodności, ale nie są one pokazywane
ani odpytywane. Zapis dwóch widocznych slotów nie usuwa pozostałych ID.
W MY23 każda strona Readings pokazuje wszystkie swoje pomiary w pierwszej
linii, także gdy zawiera dwa, trzy albo cztery pomiary. Krótszy format usuwa
zbędne spacje i upraszcza długie podpisy, aby zmieścić się w 16 znakach. Ten sam
format jest używany w ulubionych. Druga linia zapowiada następną stronę.
Gdy widać ten podgląd, BACCAble odczytuje także jej wartości diagnostyczne w tle.
Po przejściu do następnej strony od razu wykorzystuje zapamiętane wartości,
jeśli odpowiedzi już nadeszły. Obie strony wspólnie korzystają z jednego zapytania
co 500 ms; wspólny pomiar jest odczytywany tylko raz na cykl. Pełny cykl rozbudowanej
pary może trwać do czterech sekund. W tym widoku wartości diagnostyczne zachowują
ważność przez 4,5 sekundy, a odczyty z cyklicznych ramek CAN przez zwykłe trzy
sekundy. Przy szybkim przewijaniu lub braku odpowiedzi sterownika nadal może
pojawić się `--`.
Na przykład `Oil 35.0mm Q 80%` mieści się dokładnie. `B 100-200 8.54s` oznacza
najlepszy czas od 100 do 200 km/h. Na stronie czterech temperatur `O` oznacza
olej, `W` płyn chłodzący, `I` temperaturę na wejściu intercoolera, a `X` na
wyjściu. Liczba, która nie mieści się w swoim polu, jest pokazana jako `--`,
a nie ucięta w połowie.
Strony statusu Information też
zapowiadają kolejną pozycję; przy wielu kodach usterek druga linia pokazuje
następny kod.

Benzyna i diesel mają osobne zestawy. Strona niepasująca do silnika pozostaje
zapisany i jest pomijany do czasu ponownego wyboru jego profilu. Niepasujący
główna strona ukrywa cały widok. Stare katalogi i identyfikatory stron pozostają
bez zmian. Stare ulubione strony są importowane jako całe strony do Slotu 1.
Atomowe identyfikatory pomiarów z beta 19 są w pamięci zamieniane na strony:
najpierw wybierana jest strona z tym jednym pomiarem, a gdy jej nie ma, pierwsza
strona, która go zawiera. Przed zapisem sprawdź wynik konwersji; dawny dodatkowy
RPM pozostaje jako strona zgodności, lecz nowe wybory pochodzą z katalogu Readings. Zapis aktualizuje
format preferencji. Starsze firmware nie odtworzy tego układu automatycznie.

`Page favorites` i `Favorite order` są dostępne tylko w starszym profilu przed
pierwszym zapisem zestawu ulubionych. Zatwierdzone zmiany aktualizują starszą listę
stron i zestawy importowane z tej listy. Błąd zapisu lub odrzucenie nie zmieniają
żadnej z nich. MY23 i profile z zapisanymi zestawami mają tylko edytor
`Favorites`, aby nie było drugiej opcji zmieniającej inną listę. Kolejność
stron ustalasz przez zawartość slotów. Pusty Slot 1 ukrywa cały zestaw;
Slot 2 nie przesuwa się automatycznie na pozycję główną. Na MY23 listy slotów, wyboru strony,
testów IPC i Shown pages pokazują bieżącą oraz następną pozycję, także po
przejściu z końca listy na początek.
`Shown pages` i `Advanced pages` dotyczą zwykłych Readings, a nie jawnego wyboru
strony do zestawu. `Sort order` ma własny szkic i potwierdzenie zapisu.
`Auto rotate` zmienia bieżący widok co pięć sekund.

### Poziom oleju i pasek

W `Settings > Features` zapisz `Engine` i `Drivetrain`. Oba odczyty oleju są
w `Readings > Engine`, bez konieczności włączania `Advanced pages`. Dotychczasowy
benzynowy `Oil height` zachowuje zapisany identyfikator i nazywa się teraz `Oil level`.

- `Oil level`: np. `Oil 77.0mm OK`; `LOW` oznacza poziom poniżej minimum,
  `HIGH` powyżej maksimum. Obie wartości graniczne należą do `OK`.
- `Oil level bar`: zajmuje całą dostępną szerokość linii. `[` i `]` oznaczają
  minimum i maksimum, a `=` przedział między nimi. Punkt `•` wskazuje bieżący
  odczyt na MY23; starsze wyświetlacze używają `*`. `<` lub `>` oznacza wynik poza skalą.

W `Settings > Favorites` wybierz je jako dwa osobne odczyty w linii 1 i 2,
w dowolnej kolejności. MY23 wykorzystuje 16 znaków w linii 1 i 22 w linii 2;
starszy wyświetlacz ma skonfigurowaną linię 18/24 znaków. Przykład dla 2.0 I4 RWD:

```text
Oil 77.0mm OK
----[=====•======]----
```

| Engine | Drivetrain | Zakres OK (mm, wraz z granicami) |
| --- | --- | --- |
| 2.0 I4 | RWD | 72–82 |
| 2.0 I4 | Q4 | 67–77 |
| 2.2 D | RWD | 61–65 |
| 2.2 D | Q4 | 79–84 |

Zakresy referencyjne podał właściciel; nie zostały niezależnie potwierdzone w tej
zmianie. Ekran korzysta z istniejącego pomiaru wysokości oleju z ECU i nie potwierdza,
że samochód spełnia warunki poprawnego pomiaru.
`Not set` wyświetla `?` obok wartości liczbowej i `Set drivetrain` zamiast paska.
Dla `2.9 V6` nie określono zakresu: pojawia się `?` / `No oil range`.
Brak, przeterminowanie lub nieprawidłowa próbka daje `Oil -- mm`; stary punkt nie pozostaje.
Dotychczasowe ulubione pozostają zapisane. Pasek i odczyt liczbowy można ukrywać osobno.

Punkt przesuwa się po jednym znaku w obie strony. BH kończy wysyłanie każdego
kroku przed rozpoczęciem następnego. Nowy pomiar zmienia cel ruchu, bez pomijania
pośrednich pozycji. Przy pierwszym wyświetleniu oraz po powrocie z braku danych
punkt pojawia się od razu w bieżącym miejscu. Liczba i LOW/OK/HIGH nadal pokazują
najnowszy odczyt ECU; pośrednie pozycje są animacją, a nie dodatkowymi pomiarami.
Obie linie Favorites animują się niezależnie.

`Quick 20ms` oznacza odstęp między fragmentami CAN, a nie pełnymi ekranami.
Przy domyślnym `Write Full` krok MY23 wymaga 13 fragmentów, czyli około 260 ms
bez opóźnień magistrali lub radia. `Write Delta` wysyła tylko zmienione fragmenty
i również kończy krok przed przejściem dalej. Ponowienie wysyłki CAN i przerwanie
przez radio zachowują bieżący krok. Szybkość nadal wynika z `BACCAble IPC`;
animacja nie zmienia tego ustawienia. Wygląd na liczniku wymaga próby w samochodzie.


## Ustawienia: Features

Otwórz `Settings → Features`. Tabele zachowują rzeczywistą kolejność opcji.
Poniższe nagłówki grup wyjaśniają działanie mocniejszych naciśnięć góra/dół — nie
są dodatkowymi podmenu. Krótkie RES otwiera edytor z zapisaną wartością.
Dopiero w edytorze kolejne naciśnięcia lub góra/dół zmieniają przełączniki i
nazwane wartości. `LKA` pokazuje wprost `OFF` lub `ON`; pozostałe przełączniki
korzystają z symbolu zaznaczenia na liście.

Dla **Launch Nm**, **Shift RPM** i **Pedal trim** krótkie RES rozpoczyna edycję,
a góra/dół zmienia szkic. Przytrzymanie zapisuje bezpośrednio. Dwuklik otwiera
pytanie o zapis; przytrzymanie zapisuje, a kolejny dwuklik odrzuca. Przełączniki i nazwane wartości działają tak samo.
Szkic nie steruje samochodem przed udanym zapisem.

### Grupa wyświetlania

| Opcja | Wartości | Do czego służy |
| --- | --- | --- |
| `Shift light` | Wyłączona / włączona | Wysyła polecenie pokazania sygnału zmiany biegu po przekroczeniu Shift RPM. Wymaga zgodnej obsługi wskazania w trybie Race. Nie zmienia biegu za kierowcę. |
| `Shift RPM` | 1500–6000 obr./min, co 250 | Próg obrotów silnika dla sygnału zmiany biegu. |
| `MY23 display` | Wyłączona / włączona | Włącza układ MY23: 16 znaków w pierwszej linii i 22 w drugiej. OFF zachowuje starszy układ jednej linii z szerokością 18/24 znaków wybraną przy kompilacji. |
| `DPF regen alert` | Wyłączona / włączona | Włącza powiadomienia o regeneracji DPF w dieslu. Zobacz [obserwowanie DPF](#obserwowanie-dpf). |
| `Auto rotate` | Wyłączona / włączona | Automatycznie zmienia stronę bieżących odczytów co pięć sekund. |
| `Advanced pages` | Wyłączona / włączona | Dodaje techniczne i dodatkowe strony do Readings, z uwzględnieniem profilu silnika i ustawionej widoczności. |

### Grupa komfortu

| Opcja | Wartości | Do czego służy |
| --- | --- | --- |
| `Seatbelt alarm` | Wyłączona / włączona | Wysyła polecenie zmiany ustawienia sygnału niezapiętych pasów. To inna pozycja niż odczyt Seatbelt alarm. Sprawdź wynik w samochodzie; samo odłączenie BACCAble nie musi przywrócić poprzedniego ustawienia sterownika. |
| `Stop odo blink` | Wyłączona / włączona | Powstrzymuje miganie licznika przebiegu. Nie usuwa niezgodności konfiguracji, która spowodowała miganie. |
| `Park mirror` | Podmenu | Włącza ustawianie lusterek do cofania i pozwala osobno zapisać ich pozycje. Zobacz [procedurę ustawiania lusterek](#lusterka-podczas-cofania). |
| `Close windows` | OFF / 1 lock / 2 locks | Wybiera zamykanie szyb po jednym lub dwóch naciśnięciach zamykania na pilocie. OFF wyłącza funkcję. Krótsza nazwa na wyświetlaczu to `Close win`. |
| `Open windows` | OFF / 1 unlock / 2 unlocks | Wybiera otwieranie szyb po jednym lub dwóch naciśnięciach otwierania na pilocie. OFF wyłącza funkcję. Krótsza nazwa to `Open win`. |
| `QV exhaust` | Wyłączona / włączona | Udostępnia sterowanie klapami wydechu przez akcję w menu i podwójne naciśnięcie przycisku odblokowania dźwigni zmiany biegów. Wymaga zgodnego sprzętu. |
| `Rain lights` | OFF / ON; domyślnie OFF | W AUTO wysyła żądanie świateł mijania i pozycyjnych po trzech wykrytych cyklach wycieraczek w ciągu 15 sekund. Podtrzymuje żądanie przez 60 sekund od ostatniego cyklu. Wymaga zgodnego firmware C1/C2. Działanie lamp wymaga jeszcze potwierdzenia w samochodzie. |
| `Auto PDC mute` | Wyłączona / włączona | Chwilowo wycisza aktywny sygnał czujników parkowania podczas hamowania poza biegiem wstecznym. Przywraca działanie zmienione przez BACCAble po odpuszczeniu hamulca lub wybraniu wstecznego. Wymaga aktualnych danych o hamulcu, biegu i czujnikach PDC. |
| `Mute audio in R` | Wyłączona / włączona | Wycisza dźwięk systemu audio na wstecznym i przywraca go po zakończeniu cofania, jeśli to BACCAble go wyciszyło. Uwzględnia wcześniejsze wyciszenie oraz ręczne przywrócenie dźwięku. Działa niezależnie od wyciszania PDC. |


`Settings` → `Features` → `Comfort` → `Rain lights`: wybierz ON i przytrzymaj przycisk, aby zapisać.
Czasy są stałe. Każdy kolejny wykryty cykl rozpoczyna od nowa 60 sekund
podtrzymania. Jeden ruch nie uruchamia funkcji; kilka ruchów po użyciu
spryskiwacza może ją uruchomić. Funkcja reaguje na pracę wycieraczek, nie mierzy
intensywności deszczu. Bardzo rzadkie ruchy mogą nie osiągnąć progu.

Opuszczenie AUTO, wyłączenie opcji lub brak aktualnych danych albo łączności
kończy żądanie. C2 przestaje dodawać żądanie świateł mijania po odebraniu
informacji o opuszczeniu AUTO; C1 kończy żądanie pozycyjnych po kolejnej odpowiedzi
(zwykle w ciągu sekundy, a bez nowej odpowiedzi po trzech sekundach).
Zakończenie żądania oznacza zaprzestanie dodawania bitów lamp, bez wysyłania
polecenia ich zgaszenia. Fabryczne sterowanie po zmroku zachowuje więc kontrolę.
Podczas pierwszej próby na postoju obserwuj rzeczywiste przednie i tylne lampy:
wysłanie ramki CAN nie potwierdza przyjęcia żądania przez sterowniki.
Wcześniejsze próby potwierdziły zapalenie lamp po zasłonięciu czujnika lub użyciu
pokrętła; odpowiadające temu wartości CAN są znane. Pozostała próba dotyczy
włączenia przez samo BACCAble, także pierwszeństwa ręcznego przełączenia
i zakończenia podtrzymania w ciemności. Poprawka odpytywania utrzymuje aktualny
status C2 i działanie nagrywania USB przy włączonym Rain lights.

### Grupa wspomagania kierowcy

| Opcja | Wartości | Do czego służy |
| --- | --- | --- |
| `Virtual ACC` | Wyłączona / włączona | Pozwala obsługiwać ACC zgodnym panelem przycisków zwykłego tempomatu. Samochód musi już mieć potrzebny sprzęt i konfigurację ACC. Funkcja nie zastępuje radaru ani innych brakujących elementów. |
| `ACC resume` | OFF / RES / + | Wybiera automatyczne wysyłanie RES albo lekkiego naciśnięcia w górę, gdy ACC jest aktywny i utrzymuje zatrzymany samochód. Może to umożliwić ponowne ruszenie. Zobacz [obsługę ACC i HAS](#obsługa-acc-i-has). |
| `Virtual HAS` | Wyłączona / włączona | Udostępnia symulowane naciśnięcie przycisku Highway Assist przez akcję lub podwójne naciśnięcie przycisku asystenta pasa. Wymaga zgodnego i skonfigurowanego wyposażenia samochodu. |
| `LKA` | OFF / ON | **OFF** zleca wyłączenie LKA raz przy każdym włączeniu zapłonu. C1 rozpoznaje OFF → RUN w `0x1EF`, także bez uśpienia urządzenia; rozruch nie rozpoczyna drugiej sesji. Po otrzymaniu konfiguracji C2 czeka do 5 s na włączone LKA w `0x547` i puszczony przycisk manetki, potem wysyła jeden dwuklik C2 `0x384`: naciśnięcie, puszczenie, naciśnięcie, puszczenie, co około 100 ms. Początkowe raporty OFF nie kończą oczekiwania; gdy LKA jest wyłączone, dwuklik nie jest wysyłany. Naciśnięcie kierowcy anuluje oczekującą akcję. **ON** pozostawia działanie fabryczne i nie wysyła automatycznego dwukliku; nie wymusza włączenia LKA. Zapisane ustawienie działa od następnego włączenia zapłonu. Później nie ma cyklicznego nadpisywania. Dotychczasowe `Disable LKA: ON` staje się `LKA: OFF`, zachowując intencję zapisanego ustawienia. Użytkownik potwierdził jednorazowe działanie bety 24 po podłączeniu i brak działania po kolejnych cyklach zapłonu; tę poprawkę trzeba jeszcze sprawdzić wielokrotnie w samochodzie. |

### Grupa układu napędowego

| Opcja | Wartości | Do czego służy |
| --- | --- | --- |
| `Launch Nm` | 25–600 Nm, co 25 | Wartość momentu obrotowego zgłaszanego przez silnik, przy której wspomaganie startu wysyła polecenie zwolnienia wymuszonych przednich hamulców. To nie jest ograniczenie momentu ani ustawienie tuningu. |
| `Default mode` | A / N / D / Race | Przy uruchomieniu auta BH symuluje sygnał pokrętła DNA w czterobajtowej ramce `0x256`. Dla A/D/Race C1 i BH czekają na świeże raporty obrotów powyżej 400 RPM przez dwie sekundy; podczas oczekiwania menu pokazuje `Start engine`. `N` pozostawia fabryczny wybór bez ingerencji; wcześniejsze zapisane OFF staje się N bez zmiany ustawień. A/D używają wychyleń po 200 ms. Race najpierw przechodzi do D, następnie przytrzymuje kierunek D przez 2,2 s. Stan naciśnięcia jest odświeżany po fabrycznych raportach neutralnych, a podczas długiego przytrzymania także po 250 ms bez powtórzenia. Nowe naciśnięcie następuje po świeżym raporcie neutralnym i opróżnieniu nadajnika CAN. Rzeczywisty tryb musi być stabilny przez sekundę przed kolejnym gestem. Obowiązuje limit trzech gestów zmiany trybu w ciągu dwunastu sekund. Fizyczne pokrętło ma pierwszeństwo, a rozpoczęte wirtualne naciśnięcie jest zwalniane po anulowaniu, zatrzymaniu silnika lub utracie aktualnych danych. Po bramce uruchomienia C1 czeka do 20 s na BH, a potem sprawdza raporty DNA i skrzyni przez pięć sekund. `Confirmed` potwierdza świeże raporty stanu, nie działanie każdego układu. Zapis ustawień lub wyłączenie zapłonu anuluje próbę; ponowna synchronizacja łącza jej nie uzbraja. Zaktualizuj C1 i BH razem. Test w samochodzie z 24 października 2026 potwierdził działanie ustawienia `Race`; tryb `A` nie został zgłoszony osobno. |
| `Suspension` | Stiff / Soft / Mid | Ustawia zawieszenie przy następnym uruchomieniu: dla D dostępne jest `Stiff` lub `Soft`, a dla Race `Stiff` lub `Mid`. Dla A/N łagodniejszy wybór ma etykietę `Softer` i nie działa. Po sekundzie stabilnego rzeczywistego trybu i 300 ms stabilnego raportu zawieszenia BACCAble naciska przycisk na 150 ms tylko wtedy, gdy potrzebna jest zmiana, również gdy auto jest już w D/Race. Czeka na świeży raport po zwolnieniu. Jedna ponowna próba jest dozwolona dopiero po co najmniej sekundzie, przy świeżych, niezmiennych raportach przeciwnego stanu oraz zakończeniu wcześniejszej transmisji CAN. Choćby chwilowe zgłoszenie stanu docelowego albo brak/nieznana odpowiedź wykluczają tę ponowną próbę. Łącznie mogą być najwyżej dwa naciśnięcia. `No confirm` może oznaczać brak potwierdzenia zawieszenia, mimo że tryb DNA się zmienił. Ręczne użycie pokrętła lub przycisku zawieszenia przerywa automat. Wymaga zgodnego firmware C1/BH. Kody raportów po ręcznym wyborze D i Race/Mid są potwierdzone nagraniami, w tym dwoma pełnymi, opisanymi cyklami Race z 24 października 2026. Automatyczne ustawianie po uruchomieniu zostało potwierdzone w samochodzie 24 października 2026: zapisane ustawienie `Soft` jest stosowane. Ścieżka ponownej próby po nieprzyjętym impulsie nie została sprawdzona osobno. |
| `ESC/TC control` | Wyłączona / włączona | Udostępnia przełączanie kontroli stabilności i trakcji oraz obsługiwany skrót przyciskiem asystenta pasa. Samochód musi mieć odpowiednią konfigurację. |
| `Dyno action` | Wyłączona / włączona | Udostępnia `Toggle Dyno mode`. Samo włączenie tego ustawienia nie uruchamia trybu Dyno. |
| `Brake action` | Wyłączona / włączona | Udostępnia `Brake override` i `Disable launch`. Samo włączenie ustawienia nie uruchamia hamulców. |
| `AWD off action` | Wyłączona / włączona | Udostępnia `AWD off request` w samochodzie z napędem AWD. Samo włączenie nie wysyła polecenia wyłączenia napędu. |
| `Pedal mode` | OFF / Auto / Bypass / A / N / D / R / Hybrid / Kids | Wybiera sposób reakcji zgodnego, dodatkowego kontrolera pedału przyspieszenia. Zobacz [kontroler pedału](#kontroler-pedału-przyspieszenia). |
| `Pedal trim` | Od −10 do +10, co 2 | Koryguje reakcję wybranej mapy pedału. Nie oznacza procentowej zmiany mocy silnika. Tryb Kids ma własną, stałą niską korektę. |

### Grupa urządzenia

| Opcja | Wartości | Do czego służy |
| --- | --- | --- |
| `Block Start/Stop` | Wyłączona / włączona | Automatycznie wysyła polecenie wyłączenia samochodowego Start/Stop. Włączenie tej opcji oznacza blokowanie automatycznego zatrzymywania silnika. |
| `CAN routing` | Wyłączona / włączona | Udostępnia wybrane zwykłe komunikaty CAN przez mechanizm diagnostyczny BACCAble zgodnym programom diagnostycznym. To nie jest przełącznik przechwytywania przez USB. |
| `DTC clear action` | Wyłączona / włączona | Udostępnia `Clear DTCs`, czyli wysyłanie poleceń kasowania błędów do wielu sterowników. |
| `BCM fault reader` | Wyłączona / włączona | Udostępnia `Read BCM faults`, czyli odczyt błędów wyłącznie sterownika nadwozia BCM. |
| `Engine` | 2.0 I4 / 2.9 V6 / 2.2 D | Wybiera zestaw odczytów dla silnika. Zmiana usuwa pomiary przechowywane chwilowo w pamięci i tymczasowo ukrywa niepasujące ulubione. |
| `Drivetrain` | Not set / RWD / Q4 | Wybiera napęd, który wraz z `Engine` określa zakres poziomu oleju; domyślnie `Not set`. Przytrzymaj RES, aby zapisać. To nie jest ustawienie skrzyni biegów. |
| `USB mode` | OFF / CAN / ELM327 | Wybiera zwykłą pracę, binarne przechwytywanie CAN albo diagnostykę zgodną z częścią funkcji ELM327. ELM327 jest dostępny w wersjach z jego obsługą. Jednocześnie działa tylko jeden tryb. Przytrzymaj RES w edytorze, aby zapisać i zastosować zmianę, lub najpierw wykonaj dwuklik, aby zobaczyć pytanie o zapis. |

Udostępnienie funkcji i jej rzeczywiste włączenie w samochodzie to różne rzeczy.
Na przykład `Dyno action` pozwala dopiero wybrać odpowiednią akcję. Część akcji
nie pojawia się na liście, gdy ich ustawienie jest wyłączone. Oczekujące polecenie
może chwilowo blokować zmianę ustawienia, które je udostępnia.

## Korzystanie z funkcji

### Lusterka podczas cofania

Lusterka muszą przekazywać informację o swoim położeniu. Zwykłe lusterka
elektryczne bez takiej informacji mogą nie obsługiwać tej funkcji.

1. Zatrzymaj samochód w warunkach pozwalających używać regulacji lusterek.
   Otwórz `Settings → Features → Park mirror`.
2. Wybierz `Enabled`, ustaw szkic ON, wykonaj dwuklik i przytrzymaj RES,
   aby zapisać. Samo włączenie **nie zapisuje** bieżących pozycji.
3. Wejdź w `Store position`. Pojawi się `> Adjust;HOLD=save` — ustaw lusterka,
   a potem przytrzymaj RES przy pytaniu STORE, aby zlecić zapis.
4. Przyciskami samochodu ustaw oba lusterka tak, jak chcesz używać ich podczas
   cofania. Poczekaj, aż przestaną się poruszać, i przytrzymaj RES przy pytaniu STORE.
5. `Store: queued` oznacza, że polecenie trafiło do kolejki wysyłania do BH.
   Ten ekran nie otrzymuje od BH potwierdzenia zapisu. Krótkie RES zamyka
   komunikat, a dwuklik RES cofa. Jeśli wysłanie było niemożliwe z powodu zajętości,
   ponów próbę, pozostawiając lusterka w pozycjach do zapisania.
6. Przywróć normalne pozycje lusterek do jazdy i wyjdź z konfiguracji.
   Sprawdź zapisane pozycje na postoju, wybierając bieg wsteczny.

Przy pracującym silniku wybierz wsteczny i włącz lewy lub prawy kierunkowskaz,
aby ustawić lusterko po odpowiedniej stronie. BACCAble zapamiętuje normalne
pozycje, żeby później je przywrócić. Ruch może rozpocząć się po kilku sekundach.
Zmiana kierunkowskazu podczas manewru może również wybrać drugie lusterko.

Polecenie przywrócenia normalnych pozycji jest wysyłane od razu po wybraniu P,
zatrzymaniu silnika lub wyłączeniu funkcji. Pozycja N musi utrzymać się przez
około 0,5 s. Przy innych sposobach wyjścia ze wstecznego opóźnienie może wynieść
do 10 s. Krótkie przejścia przez pozycje skrzyni są celowo pomijane.
Brak aktualnych danych o położeniu lusterek lub biegu może zatrzymać działanie.
Obserwuj lusterka — komunikat o zakolejkowaniu nie dowodzi, że się poruszyły.

### Otwieranie i zamykanie szyb pilotem

Osobno wybierz jedno lub dwa naciśnięcia pilota dla otwierania i zamykania.
Jeśli używasz kilku naciśnięć, odstęp między nimi powinien wynosić najwyżej około
trzech sekund. Zamykanie zaczyna się mniej więcej cztery sekundy od ostatniego
polecenia zamknięcia auta. Poczekaj na zakończenie ruchu szyb.

**Dodatkowe naciśnięcie zamykania** powoduje wysłanie polecenia krótkiego uchylenia
szyb po ich zamknięciu: dwa naciśnięcia przy `1 lock` albo trzy przy `2 locks`.
Uchylenie zależy od czasu działania, dlatego jego wielkość zależy od samochodu.
Polecenie odblokowania auta anuluje trwające zamykanie szyb, a polecenie
zablokowania anuluje ich otwieranie. Sprawdź końcowe położenie szyb, zanim
odejdziesz od samochodu.

### Kontroler pedału przyspieszenia

Te ustawienia wymagają dodatkowego, zgodnego kontrolera pedału. Nie zmieniają
map sterownika silnika. `Pedal map` pokazuje mapę zgłoszoną przez kontroler.
Może różnić się od wybranego trybu, gdy brakuje komunikacji lub polecenie
jeszcze czeka na wykonanie.

| Tryb | Działanie |
| --- | --- |
| OFF | Przy wyłączaniu wysyła polecenie Bypass, a następnie przestaje automatycznie zarządzać mapą. |
| Auto | Dopasowuje mapę do trybu DNA samochodu: A, N, D albo R. |
| Bypass | Utrzymuje tryb obejścia kontrolera, bez jego modyfikacji reakcji pedału. |
| A / N / D / R | Wybiera stałą mapę: All Weather / Natural / Dynamic / Race. |
| Hybrid | Zwykle używa N, a po zgłoszeniu trybu Race przez samochód — R. |
| Kids | Używa A ze stałą korektą −10. Powyżej 4000 obr./min w benzynie, 3000 obr./min w dieslu albo 100 km/h wysyła polecenie minimalnego wysterowania pedału. Działanie zależy od zewnętrznego kontrolera; nie jest to gwarantowany ogranicznik prędkości. |

### Obsługa ACC i HAS

Przy włączonym `Virtual ACC` przycisk włączania zwykłego tempomatu CC wysyła
również polecenie włączenia ACC. Gdy ACC już reguluje jazdę, RES działa jak
przycisk zmiany odstępu. Gdy CC i ACC są wyłączone, RES nadal obsługuje menu.
Funkcja wymaga radaru, pozostałego potrzebnego wyposażenia i zgodnej konfiguracji
samochodu. Włączenie opcji nie zastępuje brakujących elementów.

`ACC resume: RES` lub `ACC resume: +` wysyła odpowiednie naciśnięcie, gdy ACC
jest aktywny, samochód od pewnego czasu stoi, a ACC utrzymuje hamulec.
Wybierz sposób wznawiania obsługiwany przez samochód. OFF wyłącza te automatyczne
polecenia. Kierowca nadal musi nadzorować zatrzymanie i ponowne ruszenie.

Aby użyć HAS, włącz `Virtual HAS`, a następnie wybierz
`Actions → Press HAS button` albo dwukrotnie naciśnij przycisk asystenta pasa
w ciągu około jednej sekundy. To symuluje naciśnięcie przycisku, ale nie
potwierdza włączenia Highway Assist. Sprawdź wskazanie systemu w samochodzie.

### ESC/TC, Dyno, hamulce i wspomaganie startu

Te funkcje służą do prób w kontrolowanych warunkach. Dyno może wyłączyć ABS,
ESC i kontrolę trakcji. Brake override może utrzymywać zaciśnięte przednie
hamulce. Wyświetlacz pokazuje polecenia i odpowiedzi kontrolerów, a nie pomiar
ciśnienia hamowania ani pewne potwierdzenie stanu napędu.

- **ESC/TC:** włącz `ESC/TC control`, a potem użyj `Toggle ESC/TC`. Alternatywą
  jest znany ze starszej wersji skrót: przytrzymanie obsługiwanego przycisku
  asystenta pasa przez około 2–3 s. Po reakcji puść przycisk. Akcja jest
  zablokowana, gdy Dyno działa lub oczekuje polecenie jego włączenia.
- **Dyno:** włącz `Dyno action`, zatrzymaj samochód i poczekaj, aż urządzenie
  rozpozna postój. Najpierw zwolnij wymuszone hamowanie i przywróć normalne
  ustawienie ESC/TC. Wybierz `Toggle Dyno mode` i potwierdź. Starszy skrót to
  przytrzymanie **przycisku asystenta parkowania** przez około sześć sekund;
  nie chodzi o przycisk P na dźwigni skrzyni. Aby wysłać polecenie wyjścia,
  użyj przełącznika ponownie. Zatrzymanie silnika również kasuje stan Dyno.
  Do usunięcia ostrzeżeń samochodu może być potrzebne wyłączenie i włączenie zapłonu.
- **Hamulce i start:** włącz `Brake action`, ustaw `Launch Nm` i na postoju
  uruchom Dyno. `Brake override` wysyła polecenie wymuszenia przednich hamulców.
  Gdy C2 zgłosi ten stan, zostaje uzbrojone wspomaganie startu. Potwierdzenie
  brzmi `Brake+launch? HOLD`. Wspomaganie startu wysyła polecenie zwolnienia
  hamulców po osiągnięciu ustawionego momentu obrotowego.
- **Ręczne zwolnienie:** jeśli wspomaganie startu jest uzbrojone, najpierw
  wykonaj `Disable launch`, a potem `Brake override`, aby wysłać polecenie
  zwolnienia hamulców. **Disable launch wyłącza tylko zwolnienie wyzwalane
  momentem obrotowym; samo nie zwalnia wymuszonych hamulców.** Sprawdź reakcję
  samochodu przed dalszymi czynnościami lub zakończeniem próby.

### Polecenia dla wydechu i AWD

Przy zgodnym sprzęcie klap wydechu włącz `QV exhaust`. Akcja `QV exhaust req`
przełącza między poleceniem otwarcia a powrotem do fabrycznego sterowania.
Starszy skrót to dwukrotne naciśnięcie **przycisku odblokowania dźwigni zmiany
biegów** w ciągu około jednej sekundy. `AUTO` oznacza sterowanie fabryczne,
a nie pewność, że klapy są zamknięte.

W samochodzie z AWD włącz `AWD off action`, pozostań na postoju i wykonaj
`AWD off request`. BACCAble powtarza polecenie wyłączenia AWD aż do anulowania.
Wybierz akcję ponownie i potwierdź `Stop AWD req?HOLD`, aby zakończyć wysyłanie.
`AWD req: OFF WAIT` oznacza polecenie wyłączenia AWD. Nie jest pomiarowym
potwierdzeniem, że napędzane są już tylko dwa koła.

### Obserwowanie DPF

W dieslu `DPF regen alert` włącza powiadamianie o wykrytej intensywnej fazie
regeneracji. Zależnie od opcji zawartych we wgranym oprogramowaniu używa
wskazania DPF na zestawie wskaźników, sygnału dźwiękowego lub obu naraz.
Wskazanie może utrzymywać się jeszcze przez około dziesięć komunikatów
po powrocie trybu regeneracji do NONE. Funkcja nie wymusza wypalania filtra.

Obserwuj razem `DPF load`, `DPF temp`, `DPF regen %` i `DPF regen mode`.
Load oznacza szacowane zapełnienie filtra, a procent regen — postęp regeneracji.
To dwa różne pomiary. `DPF regen mode` może pokazywać:

| Wartość na ekranie | Znaczenie |
| --- | --- |
| `NONE` / `NONE.` | W danym rozpoznanym stanie nie ma nazwanej aktywnej fazy regeneracji. |
| `DPF LO` / `DPF HI` | Tryb regeneracji DPF o niskiej / wysokiej intensywności. |
| `NSC De-NOx` | Regeneracja katalizatora magazynującego tlenki azotu — usuwanie NOx. |
| `NSC De-SOx` | Odsiarczanie katalizatora magazynującego tlenki azotu. |
| `SCR HeatUp` | Nagrzewanie układu SCR. |
| `?` | Nierozpoznany tryb. |

Lista odczytów nie rozpoznaje, czy dany samochód ma układ SCR/AdBlue.
Jeśli go nie ma, ukryj nieobsługiwane strony AdBlue.

### Stan immobilizera i przełączanie przez właściciela

`Information → Immobilizer` pokazuje zapisany stan ON/OFF zabezpieczenia
BACCAble przed programowaniem kluczy przez diagnostykę. Jest ono niezależne
od fabrycznego immobilizera. ON nie oznacza, że silnik jest właśnie zablokowany.
Działanie alarmu napadowego zależy od konfiguracji samochodu.

W wersjach pozwalających właścicielowi przełączać tę funkcję działa dotychczasowy
skrót: zamknij menu BACCAble, wyłącz CC i ACC, pozostaw pracujący silnik przy
skrzyni w N, puść przyciski kierownicy, a następnie przytrzymaj **lekkie
naciśnięcie w górę przez około 30 sekund**. Trzy mignięcia jasności zestawu
wskaźników oznaczają włączenie, a sześć — wyłączenie. Potem sprawdź Information.
Ten skrót zapisuje ustawienie osobno. Przytrzymanie kierunku wewnątrz menu
nie uruchamia tej funkcji.

## Akcje: Actions

Otwórz `Actions` w menu głównym. Jeśli akcja wymaga udostępnienia, najpierw włącz
jej ustawienie w Features. Tabela zachowuje kolejność akcji. Pozycje, których
ustawienia są wyłączone, mogą być pominięte na liście.

Z wyjątkiem `Read BCM faults` i `Peak hold` krótkie RES najpierw wyświetla
potwierdzenie. Aby wykonać akcję, przytrzymaj RES **w ciągu trzech sekund**.
Przejście do innej pozycji lub powrót anuluje potwierdzenie. Przy wykonaniu
urządzenie ponownie sprawdza wymagane warunki.

| Akcja | Ustawienie / wymagane warunki | Działanie |
| --- | --- | --- |
| `QV exhaust req` | QV exhaust; zgodny sprzęt | Wysyła polecenie otwarcia klap lub przywrócenia fabrycznego sterowania. WAIT nie potwierdza położenia klap. |
| `Press HAS button` | Virtual HAS | Wysyła symulowane naciśnięcie przycisku HAS. `HAS: Request sent` nie oznacza, że HAS się włączył. |
| `Toggle ESC/TC` | ESC/TC control; Dyno nieaktywny | Przełącza polecenie zmiany działania stabilizacji i kontroli trakcji. Potwierdzenie może pozostać niedostępne. |
| `Toggle Dyno mode` | Dyno action; odpowiednio długi postój; brak wymuszonego hamowania i zmienionego stanu ESC | Wysyła polecenie włączenia/wyłączenia Dyno. Przy włączaniu pojawia się `Dyno+ESC? HOLD`. Odpowiedź C2 kończy oczekiwanie na kontroler; sprawdź stan samochodu. |
| `Brake override` | Brake action; przed zmianą istniejącego wymuszenia trzeba wyłączyć wspomaganie startu; do włączenia wymagane są postój i aktywny Dyno | Wysyła polecenie wymuszenia przednich hamulców ze wspomaganiem startu albo przywrócenia normalnej pracy hamulców. Zobacz opisaną wyżej procedurę zwalniania. |
| `Disable launch` | Brake action; wspomaganie startu uzbrojone | Wyłącza zwolnienie hamulców wyzwalane momentem obrotowym. Nie zwalnia wymuszonego hamowania. |
| `AWD off request` | AWD off action; postój przy uruchamianiu | Rozpoczyna powtarzanie polecenia wyłączenia AWD albo anuluje jego wysyłanie. Nie daje pomiarowego potwierdzenia stanu napędu. |
| `Read BCM faults` | BCM fault reader; kasowanie DTC nie trwa | Od razu otwiera odczyt i przeglądanie błędów BCM. Nie kasuje kodów. |
| `Clear DTCs` | DTC clear action; brak równoczesnego odczytu BCM lub kasowania | Wysyła polecenia kasowania błędów do skonfigurowanych sterowników na magistralach samochodu. Zakres jest szerszy niż odczyt BCM. Nie usuwa przyczyny usterki. |
| `Reset best times` | Nie wymaga osobnego ustawienia w Features | Kasuje zapisane najlepsze czasy przyspieszenia. Wymaga potwierdzenia; udany zapis kończy komunikat `Records cleared`. |
| `Peak hold` | Nie wymaga osobnego ustawienia w Features | Od razu włącza/wyłącza pokazywanie najwyższych wartości liczbowych dla bieżącej strony. |
| `IBS SOC override` | Pracujący silnik; potwierdzenie | Włącza/wyłącza tymczasowe, eksperymentalne podmienianie SOC w komunikatach. Ograniczenia opisano niżej. |
| `Rain test` | Nie wymaga osobnego ustawienia; zgodny firmware C1/C2/BH | Otwiera dwie strony diagnostyki świateł deszczowych; góra/dół przełącza między wycieraczkami i światłami a odczytem sygnału światła z BH. SELECT na obu stronach wymusza bity lamp w `0x73E` przez maksymalnie 30 s; C2 kończy wymuszenie także po 3 s bez komend. BACK kończy test. |

Pierwsza strona `Rain test` działa jak dotąd: `Wiper` liczy zbocza ruchu
wycieraczek także przy wyłączonym `Rain lights`; pierwsza próbka po przerwie
ustala tylko stan początkowy. `A` pokazuje aktualny stan AUTO (`?` oznacza brak
danych), `ARM` aktywne żądanie świateł, `Q` liczbę ramek przyjętych do kolejki
C2 modulo 256, a `F` żądanie wymuszenia z C1. `C2: no reply` oznacza brak
świeżych danych z C2. `Q` nie potwierdza zapalenia lamp.
Druga strona pokazuje na przykład `BH738 6/6`: dwie oddzielne, **surowe**
wartości z bajtów 1 i 2 ramki BH `0x738`. Podczas zapisanych testów w świetle
dziennym zwykle było `6/6`, a po zasłonięciu czujnika `0/0`; nie są to luksy ani
skalibrowane progi jasności. `BH738 no reply` oznacza brak aktualnej odpowiedzi
BH, a `BH738 no frame` — odpowiedź BH bez świeżej ramki `0x738`. Ten odczyt
służy tylko do obserwacji i nie steruje żądaniem świateł w deszczu.

Polecenia Dyno i hamowania czekają do dziesięciu sekund na odpowiedź kontrolera.
Upływ tego czasu nie ponawia ani nie anuluje wysłanego polecenia. Odpowiedź może
nadejść później. Potwierdzenie hamowania dotyczy przebiegu wymuszenia, a nie
zmierzonego ciśnienia. Polecenia AWD i wydechu nie mają potwierdzenia na podstawie
pomiaru ich rzeczywistego stanu.

### Odczyt i kasowanie błędów

1. Włącz `BCM fault reader` w Features i wróć.
2. Wybierz `Actions → Read BCM faults`. Poczekaj na zakończenie
   `Reading faults...`, czyli odczytywania błędów.
3. Przeglądaj kody przyciskami góra/dół. Mają postać np. `Uxxxx-xx`.
   Ekran nie podaje opisu naprawy. Pokazuje maksymalnie **20 kodów**.
   Znak `+` przed kodem oznacza, że odpowiedź zawierała więcej błędów,
   niż mieści się na liście.
4. Krótkie RES rozpoczyna kolejny odczyt. Dwuklik RES wychodzi i przerywa
   niezakończony odczyt.

`No faults reported` oznacza brak błędów w tej poprawnie odebranej odpowiedzi BCM,
a nie brak błędów w całym samochodzie. `× CAN send failed` oznacza błąd wysyłania,
`× Read timeout` — przekroczenie czasu odczytu, `× ECU rejected` — odrzucenie
przez sterownik, a `× Invalid reply` — niepoprawną odpowiedź. Żaden z tych
komunikatów nie oznacza „brak błędów”.

Przed kasowaniem zapisz potrzebne kody. Włącz `DTC clear action`, wybierz
`Clear DTCs` i potwierdź. `DTC: Request sent` oznacza zakończenie wysyłania
sekwencji do wielu sterowników. Nie potwierdza, że każdy ją przyjął ani że
usunięto przyczynę problemu. Aby sprawdzić wynik, odczytaj właściwe sterowniki
ponownie odpowiednim narzędziem diagnostycznym.

### Wartości maksymalne i wyniki przyspieszenia

`Peak hold` zachowuje najwyższą wartość każdego pola liczbowego od rozpoczęcia
obserwacji bieżącej strony. Zmiana strony lub przełączenie tej funkcji zaczyna
obserwację od nowa. Liczy się największa liczba z uwzględnieniem znaku,
a nie największa wartość bezwzględna. Pola stanu nadal pokazują bieżące dane.
Nieaktualny lub niedostępny sygnał daje `--`, a nie dawne maksimum.
Aby wrócić do zwykłych odczytów na żywo, wyłącz Peak hold. Automatyczna rotacja
zmienia strony, więc również rozpoczyna obserwację maksimum od nowa.

Pomiar przyspieszenia korzysta z prędkości zgłaszanej przez samochód.
`0-100 km/h` zaczyna pomiar przy ruszaniu i kończy po osiągnięciu górnego progu.
`100-200 km/h` korzysta z odpowiednich wyższych progów. `RUN` oznacza trwającą
próbę, a `MISS` — brak poprawnego wyniku w przewidzianym czasie, odpowiednio
20 s i 40 s. Zakończony wynik jest podawany w sekundach. `Best 0-100` oraz
`Best 100-200` zachowują najlepsze zapisane wyniki również po ponownym
uruchomieniu, aż do ich skasowania. To pomiar na podstawie sygnałów samochodu,
a nie certyfikowany pomiar zewnętrznym przyrządem.

### IBS SOC override

Ta eksperymentalna akcja **wysyła zmienione komunikaty czujnika akumulatora**.
Gdy obserwowany SOC z IBS wynosi co najmniej 75%, ale mniej niż 98%, przez
krótki czas powtarza komunikaty z SOC zastąpionym wartością 75%.
Nie jest to potwierdzony sposób poprawy ładowania i nie naprawia braku odczytu SOC.

Po uruchomieniu urządzenia funkcja jest wyłączona. Zatrzymuje się po zgaszeniu
silnika i jest anulowana przy wejściu w tryb diagnostyczny. Samo oglądanie stron
SOC nigdy jej nie włącza. Przed zmianą trybu USB wyłącz tę funkcję; w przeciwnym
razie menu pokaże `! Stop IBS first` — najpierw zatrzymaj podmienianie IBS.

## Informacje: Information

Strony ze stanem służą tylko do odczytu. Lekkie naciśnięcia góra/dół przełączają
między nimi; krótkie RES nie zmienia ich stanu. Ostatnia pozycja `Demo` uruchamia
pokaz po naciśnięciu RES.

| Strona, według kolejności | Znaczenie |
| --- | --- |
| Wersja C1 | Wersja oprogramowania kontrolera obsługującego menu. |
| Wersja C2 | Wersja kontrolera podwozia. `? C2 no reply` oznacza brak aktualnej odpowiedzi z wersją. |
| Wersja BH | Wersja kontrolera nadwozia. `? BH no reply` oznacza brak aktualnej odpowiedzi z wersją. |
| `MY23:ON/OFF …ch` | Starszy profil: ustawienie MY23 i szerokość 18/24 znaki. MY23 pokazuje `IPC MY23` oraz `16 / 22 glyphs`. |
| `Immobilizer: ON/OFF` | Zapisane ustawienie immobilizera BACCAble. Nie można go przełączyć na tej stronie. |
| `Reports:` | Liczba komunikatów o przyciskach kierownicy odebranych przez menu. Pomaga sprawdzić, dlaczego przyciski nie reagują. |
| `Gaps:` | Liczba wykrytych przerw w docieraniu komunikatów o przyciskach. |
| `Max gap:…ms` | Najdłuższy zaobserwowany odstęp między komunikatami o przyciskach, w milisekundach. |
| `Input age:…ms` | Czas od ostatniego komunikatu o przyciskach. `--` oznacza, że nie ma takiego odczytu. |
| `> Demo` | Uruchamia pokaz menu i przykładowych wartości. |

Odpowiedzi z wersją C2/BH starsze niż około pięć sekund są uznawane za nieaktualne.
Liczniki dotyczą obsługi menu, **nie liczby ramek zapisanych podczas przechwytywania
CAN**.

Wybierz `Information → Demo`, aby uruchomić pokaz. Urządzenie przejdzie kolejno
przez Favorites, Readings, Actions i Features. Pokaże przykładowe odczyty oleju
i temperatur oraz pozorną zmianę Rain lights, Default mode i Suspension. Są to
tylko przykłady na ekranie: pokaz nie zmienia ustawień, nie zapisuje Flash ani
nie wysyła poleceń do samochodu. Dwuklik RES kończy pokaz i wraca do Information;
pozostałe naciśnięcia nie obsługują wyświetlanych pozycji. Po ostatnim ekranie
powrót następuje automatycznie. Wartości takie jak `77 mm` i `2500 rpm` nie są
bieżącymi pomiarami samochodu.


## Jak rozumieć odczyty

Wybierz `Readings`, grupę i stronę. Podpis przy bieżącej wartości może być krótszy
niż nazwa na liście Favorites/Shown pages. Wszystkie odczyty opisane poniżej są
w katalogu, ale nieobsługiwane zapytania do sterownika ECU mogą stale dawać `--`.

- **Power** to moc obliczona z podawanego momentu i obrotów silnika. Jednostka
  `PS` to metryczne konie mechaniczne, czyli polskie KM; w starej instrukcji
  używano oznaczenia CV. To nie jest pomiar z hamowni.
- **Objętość, poziom, jakość, ciśnienie i temperatura oleju to różne parametry.**
  Jakość jest oceną sterownika, a nie wynikiem badania laboratoryjnego.
  Poziom oleju oceniaj w warunkach pomiaru zalecanych przez producenta pojazdu.
- **Ignition cyl 1–6** pokazuje korektę zapłonu dla danego cylindra w stopniach.
  Nie jest licznikiem wypadania zapłonów. Strony wypadania zapłonów dla V6
  obejmują tylko cylindry 1–4, ponieważ katalog nie zawiera potwierdzonych
  definicji indywidualnych liczników dla cylindrów 5 i 6.
- **Distance (ECU)** to odczyt diagnostyczny opisany w starej instrukcji jako
  dystans od wyzerowania licznika ECU. Bez sprawdzenia interpretacji danego
  sterownika nie utożsamiaj go z całkowitym przebiegiem na zestawie wskaźników.
- **Gear** pokazuje N, R lub numer biegu; DNA pokazuje A, N, D lub R.
  Brak rozpoznanego stanu może dawać `--`/`?`. `Pedal map` korzysta z odpowiedzi
  zewnętrznego kontrolera pedału.
- W tabelach wartości na stronach łączonych opisano **od lewej do prawej**.
  Na zwartej stronie temperatur `O` oznacza olej, `W` — płyn chłodzący,
  `I`/`In` — wlot intercoolera, a `X`/`Out` — jego wylot. Wszystkie temperatury
  na tej stronie są w stopniach Celsjusza.

### Naładowanie akumulatora: z którego źródła pochodzi SOC?

SOC oznacza stopień naładowania akumulatora. Oprogramowanie udostępnia kilka
niezależnych źródeł tej wartości:

| Strona lub pole | Źródło i znaczenie |
| --- | --- |
| `Batt charge BCM` → `Batt BCM SOC` (benzyna) | Diagnostyczny odczyt SOC ze sterownika nadwozia BCM. |
| `Batt charge / A` (benzyna i diesel) | Diagnostyczny SOC ze sterownika silnika, obok prądu akumulatora odczytanego ze zwykłych komunikatów CAN. |
| `Battery charge` (diesel) | SOC ze zwykłego komunikatu czujnika akumulatora IBS. |
| `Battery sources` (oba katalogi, strona zaawansowana) | Najpierw SOC z IBS, potem SOC ze sterownika silnika. Obie wartości są w procentach. |
| `Battery IBS raw` | Surowe bajty 0 i 1 komunikatu IBS pokazane jako liczby. To nie są dwa odczyty procentowe. |
| `IBS raw0/SOC/V`, `IBS raw1/SOC/V` | Jeden surowy bajt, SOC z BCM i napięcie z BCM. Te strony serwisowe korzystają z definicji BCM także w katalogu diesla; mogą nie być obsługiwane. |

Dlatego `Batt charge / A` może pokazywać `--%` mimo obecnego odczytu prądu,
a `Batt charge BCM` może być niedostępny, mimo że działa SOC z IBS.
Urządzenie nie zastępuje automatycznie jednego źródła drugim. Brak SOC nie
oznacza rozładowanego akumulatora. Użyj `Battery sources`, aby porównać
dostępność źródeł, oraz `Battery V / A`, aby osobno obserwować napięcie i prąd.

Zgodnie z przyjętą konwencją dodatni prąd oznacza ładowanie, a ujemny —
rozładowywanie. Samo napięcie nie dowodzi ładowania. Strony z surowymi bajtami IBS
służą do diagnostyki źródła danych. Nie odczytuj całego bajtu jako SOC bez
uwzględnienia jego znaczników i sposobu dekodowania.

### Oznaczenia w wykazach

**A** w ostatniej kolumnie oznacza stronę zaawansowaną: zwykle jest ukryta,
dopóki nie włączysz `Advanced pages`. **—** oznacza zwykłą stronę, nadal zależną
od ustawionej widoczności i zgodności z silnikiem.
Skróty grup: **Eng** = Engine, silnik; **Temp** = Temperatures, temperatury;
**Batt** = Battery, akumulator; **DPF** = DPF / AdBlue; **Perf** = Performance,
osiągi; **Other** = pozostałe. Oba zestawy są też dostępne przez All readings.

## Odczyty dla silników benzynowych

Poniżej znajdują się wszystkie **64 strony benzynowe**. Jeśli nie zaznaczono
inaczej, dotyczą zarówno 2.0 I4, jak i 2.9 V6. Tabela zachowuje kolejność katalogu.
Przy przeglądaniu według grup lub A–Z kolejność może być inna.

W opisach „z komunikatów CAN” oznacza wartość pobieraną ze zwykłych komunikatów
samochodu. „Odczyt diagnostyczny” oznacza wartość uzyskaną przez zapytanie
do sterownika. Ten sam parametr z dwóch takich źródeł może mieć różną dostępność.

| Nazwa strony | Grupa | Jednostki i znaczenie | A |
| --- | --- | --- | --- |
| Power / torque | Perf | PS / Nm; obliczona moc i moment obrotowy zgłaszany przez silnik. | — |
| Oil bar/coolant | Temp | bar / °C; ciśnienie oleju z komunikatów CAN i temperatura płynu chłodzącego z ECU. | A |
| Oil bar / temp | Temp | bar / °C; ciśnienie i temperatura oleju z komunikatów CAN. | A |
| Oil/coolant temp | Temp | °C / °C; temperatura oleju z komunikatów CAN i temperatura płynu chłodzącego z ECU. | — |
| Oil level/qual. | Eng | L / %; objętość oleju i ocena jego jakości ze sterownika silnika. | A |
| Batt charge / A | Batt | % / A; SOC ze sterownika silnika i prąd akumulatora ze znakiem. | A |
| Battery V / A | Batt | V / A; napięcie z BCM i prąd akumulatora ze znakiem. | — |
| Power | Perf | PS; moc obliczona z momentu obrotowego i obrotów silnika. | — |
| Torque | Perf | Nm; moment obrotowy zgłaszany przez silnik. | — |
| Intercooler out | Temp | °C; temperatura powietrza na wylocie intercoolera, czyli chłodnicy powietrza doładowującego. | — |
| Intercooler in | Temp | °C; temperatura powietrza na wlocie intercoolera. | — |
| Intake abs press | Eng | bar; ciśnienie bezwzględne w dolocie, obejmujące ciśnienie atmosferyczne. | A |
| Boost pressure | Eng | bar; ciśnienie bezwzględne w dolocie pomniejszone o stały poziom odniesienia 1 bar. Wynik może być ujemny. | — |
| Turbo sensor V | Eng | V; napięcie sygnału czujnika ciśnienia turbo/dolotu. To nie jest ciśnienie doładowania. | A |
| Distance (ECU) | Perf | km; diagnostyczny licznik dystansu z ECU. | A |
| Oil volume | Eng | L; objętość oleju zgłaszana przez ECU. | — |
| Oil pressure | Eng | bar; diagnostyczny odczyt ciśnienia oleju z ECU. Źródło inne niż na stronach łączonych z ciśnieniem z komunikatów CAN. | — |
| Oil temp (ECU) | Temp | °C; diagnostyczny odczyt temperatury oleju z ECU. | — |
| Oil quality | Eng | %; ocena jakości oleju według ECU. | A |
| MultiAir temp | Temp | °C; temperatura oleju w module MultiAir. **Tylko I4.** | — |
| Gearbox temp | Temp | °C; temperatura skrzyni biegów. | — |
| Batt charge BCM | Batt | %; SOC akumulatora z BCM. Podpis bieżącego odczytu: `Batt BCM SOC`. | — |
| Battery current | Batt | A; prąd akumulatora z komunikatów CAN, ze znakiem. | — |
| Battery voltage | Batt | V; diagnostyczny odczyt napięcia akumulatora z BCM. | — |
| A/C pressure | Other | bar; ciśnienie czynnika w układzie klimatyzacji. | A |
| Gear | Eng | N / R / numer; zgłoszony aktualny bieg. | — |
| Engine run time | Eng | min; zgłoszony czas od uruchomienia silnika. | A |
| Over-rev time | Eng | s; czas pracy przy nadmiernych obrotach, zgłoszony przez ECU. | A |
| Over-rev count | Eng | Liczba zgłoszonych przez ECU przypadków nadmiernych obrotów. | A |
| Exhaust temp | Temp | °C; temperatura spalin według ECU. | — |
| Catalyst temp | Temp | °C; temperatura katalizatora. | — |
| Coolant temp | Temp | °C; temperatura płynu chłodzącego silnik. | — |
| Knock sensor | Eng | mV; sygnał czujnika spalania stukowego. To nie jest licznik zdarzeń spalania stukowego. | A |
| Key ID | Other | Liczbowy identyfikator klucza z diagnostyki. Nie jest pełnym identyfikatorem klucza w zapisie szesnastkowym. | A |
| Ignition cyl 1 | Eng | °; korekta zapłonu cylindra 1. | — |
| Ignition cyl 2 | Eng | °; korekta zapłonu cylindra 2. | — |
| Ignition cyl 3 | Eng | °; korekta zapłonu cylindra 3. | — |
| Ignition cyl 4 | Eng | °; korekta zapłonu cylindra 4. | — |
| DNA mode | Other | A / N / D / R; zgłoszony tryb jazdy. | — |
| Speed | Perf | km/h; prędkość zgłaszana przez samochód. | — |
| Seatbelt alarm | Other | ON / OFF; zgłoszone ustawienie dźwiękowego ostrzegania o pasach. | A |
| 0-100 km/h | Perf | s, RUN albo MISS; bieżąca lub ostatnia próba 0–100. | — |
| 100-200 km/h | Perf | s, RUN albo MISS; bieżąca lub ostatnia próba 100–200. | — |
| Best 0-100 | Perf | s; zapisany najlepszy czas 0–100. | — |
| Best 100-200 | Perf | s; zapisany najlepszy czas 100–200. | — |
| Pedal map | Other | B / A / N / D / R; mapa zgłoszona przez kontroler pedału albo znacznik braku odczytu. | — |
| Ignition cyl 5 | Eng | °; korekta zapłonu cylindra 5. **Tylko V6.** | — |
| Ignition cyl 6 | Eng | °; korekta zapłonu cylindra 6. **Tylko V6.** | — |
| Oil level | Eng | mm; wysokość oleju z ECU i LOW / OK / HIGH dla wybranego silnika i napędu. | — |
| Oil level bar | Eng | Pasek wysokości oleju na całą linię z granicami zakresu. | — |
| Misfires total | Eng | Całkowita liczba wypadnięć zapłonu zgłoszona przez ECU. | — |
| Misfires cyl 1 | Eng | Liczba wypadnięć zapłonu w cylindrze 1. | A |
| Misfires cyl 2 | Eng | Liczba wypadnięć zapłonu w cylindrze 2. | A |
| Misfires cyl 3 | Eng | Liczba wypadnięć zapłonu w cylindrze 3. | A |
| Misfires cyl 4 | Eng | Liczba wypadnięć zapłonu w cylindrze 4. | A |
| Misfires cyl 1-4 | Eng | Cztery liczniki, kolejno dla cylindrów 1, 2, 3 i 4. W V6 obejmują tylko część cylindrów. | — |
| Oil L / mm / % | Eng | L / mm / %; objętość oleju, wysokość jego poziomu i ocena jakości. | A |
| Oil/water/IC C | Temp | °C × 4; temperatury: oleju z ECU, płynu chłodzącego, wlotu i wylotu intercoolera. | A |
| Oil/water/gear C | Temp | °C × 3; temperatury: oleju z ECU, płynu chłodzącego i skrzyni biegów. | A |
| Battery sources | Batt | % / %; SOC z komunikatu IBS i SOC ze sterownika silnika. | A |
| Battery IBS raw | Batt | Dwa surowe bajty IBS w zapisie dziesiętnym: najpierw bajt 0, potem bajt 1. Bez jednostki fizycznej. | A |
| IBS raw0/SOC/V | Batt | Surowy bajt 0 / % / V; dane IBS, SOC z BCM i napięcie z BCM. | A |
| IBS raw1/SOC/V | Batt | Surowy bajt 1 / % / V; dane IBS, SOC z BCM i napięcie z BCM. | A |
| Oil volume/mm | Eng | L / mm; objętość oleju i wysokość jego poziomu. | A |
| Misfires/MA temp | Eng | Liczba / °C; całkowita liczba wypadnięć zapłonu i temperatura oleju MultiAir. **Tylko I4.** | A |

## Odczyty dla silnika Diesla

Poniżej znajdują się wszystkie **60 stron dla diesla**, dostępne w profilu 2.2 D.
Odczyty AdBlue wymagają dodatkowo wyposażenia w SCR. Niektóre techniczne zapytania
diagnostyczne mogą nie być obsługiwane przez daną wersję sterownika.

| Nazwa strony | Grupa | Jednostki i znaczenie | A |
| --- | --- | --- | --- |
| Power / torque | Perf | PS / Nm; obliczona moc i moment obrotowy zgłaszany przez silnik. | — |
| Oil bar/coolant | Temp | bar / °C; ciśnienie oleju z komunikatów CAN i temperatura płynu chłodzącego z ECU. | A |
| Oil bar / temp | Temp | bar / °C; ciśnienie i temperatura oleju z komunikatów CAN. | A |
| Oil/coolant temp | Temp | °C / °C; temperatura oleju z komunikatów CAN i temperatura płynu chłodzącego z ECU. | — |
| Oil level/qual. | Eng | mm / %; wysokość poziomu oleju i ocena jakości. W przeciwieństwie do benzyny pierwsza wartość nie jest w litrach. | A |
| Batt charge / A | Batt | % / A; SOC ze sterownika silnika i prąd akumulatora ze znakiem. | A |
| Battery V / A | Batt | V / A; diagnostyczny odczyt napięcia i prąd akumulatora ze znakiem. | — |
| DPF load / temp | DPF | % / °C; szacowane zapełnienie filtra i temperatura DPF. | — |
| DPF regen / temp | DPF | % / °C; postęp regeneracji i temperatura DPF. | A |
| Power | Perf | PS; moc obliczona z momentu obrotowego i obrotów silnika. | — |
| Torque | Perf | Nm; moment obrotowy zgłaszany przez silnik. | — |
| DPF load | DPF | %; szacowane zapełnienie filtra cząstek stałych. | — |
| DPF temp | DPF | °C; temperatura filtra cząstek stałych. | — |
| DPF regen % | DPF | %; postęp regeneracji, a nie zapełnienie filtra. | — |
| DPF regen mode | DPF | Rozpoznana faza regeneracji lub pracy układu oczyszczania spalin. Zobacz rozdział o DPF. | — |
| Since DPF regen | DPF | km; dystans od ostatniej regeneracji. | — |
| DPF regen count | DPF | Liczba regeneracji zgłoszona przez ECU. | A |
| DPF avg interval | DPF | km; średni dystans między regeneracjami według ECU. | A |
| DPF avg duration | DPF | min; średni czas regeneracji według ECU. | A |
| Battery voltage | Batt | V; diagnostyczny odczyt napięcia akumulatora dla diesla. | — |
| Battery charge | Batt | %; SOC akumulatora z komunikatu IBS. | — |
| Battery current | Batt | A; prąd akumulatora z komunikatów CAN, ze znakiem. | — |
| Oil quality | Eng | %; ocena jakości oleju według ECU. | A |
| Oil temp | Temp | °C; temperatura oleju silnikowego z komunikatów CAN. | — |
| Oil pressure | Eng | bar; ciśnienie oleju silnikowego z komunikatów CAN. | — |
| Oil level | Eng | mm; wysokość oleju z ECU i LOW / OK / HIGH dla wybranego silnika i napędu. | — |
| Oil level bar | Eng | Pasek wysokości oleju na całą linię z granicami zakresu. | — |
| AdBlue volume | DPF | L; ilość AdBlue. **Wymagany układ SCR.** | — |
| AdBlue level | DPF | %; poziom AdBlue w zbiorniku. **Wymagany układ SCR.** | — |
| Gearbox temp | Temp | °C; temperatura skrzyni biegów. | — |
| Exhaust temp | Temp | °C; temperatura spalin. | — |
| Gear | Eng | N / R / numer; zgłoszony aktualny bieg. | — |
| Coolant temp | Temp | °C; temperatura płynu chłodzącego silnik. | — |
| EGR target | Eng | %; zadana wartość sterowania recyrkulacją spalin EGR. | A |
| EGR actual | Eng | %; zgłoszona rzeczywista wartość EGR. | A |
| Turbo target bar | Eng | bar; docelowe ciśnienie turbo według ECU. | A |
| Turbo target % | Eng | %; docelowa wartość sterowania turbo według ECU. | A |
| Turbo temp | Temp | °C; temperatura z odczytu diagnostycznego związanego z turbo. | — |
| Turbo actual bar | Eng | bar; rzeczywiste ciśnienie turbo według ECU. | — |
| Turbo actual % | Eng | %; rzeczywista wartość sterowania turbo według ECU. | A |
| Boost target | Eng | bar; osobny odczyt docelowego ciśnienia doładowania z ECU. | — |
| Intake sensor V | Eng | V; napięcie sygnału czujnika dolotu. | A |
| Fuel pressure | Eng | bar; ciśnienie paliwa. | — |
| Fuel temp | Temp | °C; temperatura paliwa. | — |
| Distance (ECU) | Perf | km; diagnostyczny licznik dystansu z ECU. | A |
| A/C pressure | Other | bar; ciśnienie czynnika w układzie klimatyzacji. | A |
| Fuel rate | Eng | L/h; chwilowe zużycie paliwa w litrach na godzinę, nie w L/100 km. | — |
| Intake air temp | Temp | °C; temperatura powietrza dolotowego. | — |
| Speed | Perf | km/h; prędkość zgłaszana przez samochód. | — |
| Seatbelt alarm | Other | ON / OFF; zgłoszone ustawienie dźwiękowego ostrzegania o pasach. | A |
| 0-100 km/h | Perf | s, RUN albo MISS; bieżąca lub ostatnia próba 0–100. | — |
| 100-200 km/h | Perf | s, RUN albo MISS; bieżąca lub ostatnia próba 100–200. | — |
| Best 0-100 | Perf | s; zapisany najlepszy czas 0–100. | — |
| Best 100-200 | Perf | s; zapisany najlepszy czas 100–200. | — |
| DNA mode | Other | A / N / D / R; zgłoszony tryb jazdy. | — |
| Pedal map | Other | B / A / N / D / R; mapa zgłoszona przez kontroler pedału albo znacznik braku odczytu. | — |
| Battery sources | Batt | % / %; SOC z komunikatu IBS i SOC ze sterownika silnika. | A |
| Battery IBS raw | Batt | Dwa surowe bajty IBS w zapisie dziesiętnym: najpierw bajt 0, potem bajt 1. Bez jednostki fizycznej. | A |
| IBS raw0/SOC/V | Batt | Surowy bajt 0 / % / V; dane IBS oraz definicje SOC i napięcia BCM, które mogą być nieobsługiwane w dieslu. | A |
| IBS raw1/SOC/V | Batt | Surowy bajt 1 / % / V; dane IBS oraz definicje SOC i napięcia BCM, które mogą być nieobsługiwane w dieslu. | A |
| Oil/water/gear C | Temp | °C × 3; temperatura oleju z komunikatów CAN, płynu chłodzącego z ECU i skrzyni biegów. | A |

## Przechwytywanie CAN i diagnostyka przez USB

C1 rezerwuje 96 KiB na aplikację i 20 KiB na opcjonalny dysk USB.
Standardowe firmware C1 korzysta z szeregowego USB i **nie udostępnia tego dysku**.
Przy aktualizacji własnej kompilacji z dyskiem 16 KiB skopiuj pliki przed
wgraniem: zmienia się adres początku dysku i może być potrzebne formatowanie.
Ustawienia zachowują dotychczasowy adres w pamięci. Nagrania CAN są zapisywane
na komputerze, więc ich pojemność się nie zmienia.

USB nie jest potrzebne do zwykłej obsługi menu. Tryby USB działające w aplikacji
różnią się od **bootloadera DFU** układu STM32, który służy do wgrywania
oprogramowania. Widoczność urządzenia w `dfu-util --list` nie oznacza, że w DFU
powinien istnieć port szeregowy do przechwytywania CAN.

### Przechwytywanie CAN

1. Podłącz do komputera przewody USB kontrolerów tych magistral, które chcesz
   rejestrować. Komunikaty CAN pochodzą z zasilanego samochodu. Możesz użyć
   sprawnego huba USB przesyłającego dane. Podłącz przewody przed włączeniem
   trybu. Jeśli komputer ma zamontowany dysk USB BACCAble, najpierw bezpiecznie
   go wysuń.
2. Wybierz `Settings → Features → USB mode`, wybierz szkic CAN, wykonaj dwuklik
   dla potwierdzenia i przytrzymaj RES, aby zapisać.
   C1 przekazuje ustawienie przechwytywania do C2 i BH. Nie trzeba wybierać
   tego trybu oddzielnie w trzech menu.
3. Rozpoznaj porty widoczne w systemie. W macOS mają postać `/dev/cu.usbmodem…`.
   C1 to układ napędowy, C2 — podwozie, BH — nadwozie. Korzystaj z identyfikacji
   produktu i numeru seryjnego USB oraz wyniku Lab doctor, jeśli te informacje
   są dostępne. Sama numeracja ścieżek nadana przez system nie potwierdza roli
   kontrolera.
4. W BACCAble Lab jawnie przypisz porty do ról. Możesz rejestrować jedną, dwie
   lub trzy magistrale. Po zainstalowaniu Lab i aktywowaniu jego środowiska:

   ```sh
   baccable doctor
   baccable capture --port C1=/dev/cu.usbmodemXXXX --no-obd --no-voice
   ```

   Zastąp przykładową ścieżkę rzeczywistą. Aby dodać magistrale, dopisz
   `--port C2=/dev/cu.usbmodemYYYY` oraz `--port BH=/dev/cu.usbmodemZZZZ`.
   Doctor sprawdza konfigurację po stronie komputera. Nie włącza trybu USB
   w samochodzie i nie rozpoczyna nagrywania.
5. Sprawdź, czy z każdej wybranej magistrali przybywają ramki i czy liczniki
   strat lub błędów nie wskazują problemu. Opisuj albo oznaczaj wykonywane
   czynności w samochodzie, aby później powiązać je ze zmianami danych CAN.
   Zakończ przechwytywanie zwykłym poleceniem wyjścia z Lab lub Ctrl-C
   i sprawdź podsumowanie sesji.
6. Przechwytywanie prowadzą **scenariusze**: ekran pokazuje jedną kontrolowaną
   procedurę naraz (po polsku, terminami z samochodu), a każdy krok
   potwierdzasz klawiszem `ENTER`/`SPACJA` (zrobione), `x` (nieudane) lub `k`
   (pomiń ten krok). Krok, którego nie chcesz wykonać — na przykład prowokowanie
   ABS, ESC albo kontroli trakcji — pomijasz klawiszem `k`; nie zapisuje on
   znacznika, więc nigdy nie wygląda jak dowód, że akcja się odbyła. `o` pomija
   całą procedurę, a `e` pomija ją i przestaje ją w ogóle planować (to samo z
   terminala: `baccable scenarios --exclude ID` / `--include ID`, zapisane w tym
   samym pliku stanu). Zatrzymany scenariusz wznawia się klawiszem `r`, `u` cofa
   ostatnie potwierdzenie, `s` przełącza do innej procedury, a `q` (lub
   `Ctrl-C`) kończy sesję. Klawisze zawsze działają w scenariuszu: `p`
   zatrzymuje, `n` dodaje notatkę, `u` cofa, a `q` wychodzi. Kolory, pasek postępu i krótki dźwięk ułatwiają
   odczyt z samochodu (`--caps`, `--no-color`, `--no-bell`). Procedury idą
   w kolejności funkcjonalnej (postój, potem jazda na placu, potem wspomaganie
   na drodze) i same przechodzą do następnej; postęp jest zapisywany w
   `config/scenario_state.json`, więc kolejne uruchomienie kontynuuje od miejsca
   przerwania. Listę procedur i postęp wypisuje `baccable scenarios`; szczegóły
   opisuje [przewodnik po scenariuszach](../baccable_lab/docs/SCENARIOS.md).

Po włączeniu trybu urządzenie czeka około dziesięciu sekund na zestawienie
połączenia USB z komputerem. Każdy kontroler pomocniczy potrzebuje własnego
połączenia. Niepodłączony może opuścić tryb przechwytywania, podczas gdy
podłączone nadal pracują. C1 śledzi też połączenia pozostałych kontrolerów,
więc rejestracja samego C2 lub BH może utrzymywać koordynację przechwytywania.
Jeśli potrzebny port już zniknął po upływie czasu, podłącz ponownie przewody
i ponownie zastosuj tryb CAN. Do rejestracji jednej magistrali nie są potrzebne
wszystkie trzy porty.

Tryb CAN wysyła **strumień binarny**, a nie tekst SLCAN. Nie zapisuje też
automatycznie danych na dysku USB urządzenia. Podczas przechwytywania zwykłe
funkcje samochodowe BACCAble nadal działają, więc zapis może obejmować również
komunikaty diagnostyczne wysyłane przez BACCAble lub inne narzędzie.
Lab zachowuje surowe strumienie i odczytane z nich rekordy w osobnej sesji
z plikiem `session.sqlite3`. Nowe przechwytywanie nie nadpisuje poprzedniej sesji.
Program zapisuje identyfikatory, zawartość ramek i czasy. Nie nadaje automatycznie
zrozumiałych nazw nieznanym sygnałom. Znaczniki strat lub błędy portów oznaczają,
że trzeba sprawdzić kompletność nagrania.

Instalację, obsługę sesji i sprawdzanie zapisu opisują:
[instrukcja Lab](../baccable_lab/README.md),
[przygotowanie sprzętu](../baccable_lab/docs/HARDWARE_SETUP.md) oraz
[instrukcja przechwytywania](../baccable_lab/docs/CAPTURE.md).

Jeśli mimo podłączenia wszystkich kabli przed włączeniem CAN pojawia się tylko C1,
sprawdź zgodność wersji firmware oraz raporty stanu C2/BH. Łącze między kontrolerami
przenosi też tekst menu: jego awaria może ukryć menu, chociaż USB C1 nadal działa.

### Diagnostyka wersji testowej z poprawką USB

W wersji testowej z poprawką USB C1 wysyła polecenia oddzielnie do C2 i BH i czeka
na ich potwierdzenia. Pojawienie się wszystkich portów może potrwać kilka sekund.
Uruchomienie USB ma najwyżej trzy próby; błąd nie zatrzymuje zwykłej obsługi menu.
Aby spróbować ponownie, jawnie zapisz OFF, a następnie jawnie zapisz CAN. Wgraj wszystkie trzy obrazy z tego samego zestawu;
starszy firmware C2/BH nie obsługuje nowego protokołu.

Po zaktualizowaniu Lab i zainstalowaniu opcjonalnych zależności `usb-status` uruchom:

```sh
baccable doctor --usb-status --samples 3
```

Polecenie odczytuje stan bez otwierania portu przechwytywania. `pending_ack` podaje
kontrolery, które nie potwierdziły trybu. `fresh: false` oznacza stary odczyt albo
brak odpowiedzi. `usb_stage: ready` oznacza uruchomienie USB, a `configured: true`
— skonfigurowanie go przez komputer. `last_usb_error` zachowuje ostatni błąd aż do
restartu, także po udanym odzyskaniu połączenia. W bezpośrednim odczycie z kontrolera
`main_loop_ms` powinien rosnąć między próbkami. Sam nieruchomy ekran nie rozstrzyga,
czy zatrzymał się C1, czy BH przestał otrzymywać nowy tekst.

Przed zmianą trybu wysuń zamontowane dyski. Ostrzeżenie macOS o odłączeniu dysku
nie potwierdza błędu CDC; sprawdź nowe porty i raport stanu. Instalację, diagnostykę
i próbę powtarzalności opisuje
[audyt USB i test w samochodzie](../docs/architecture/USB_DIAGNOSTICS.md#msc-to-cdc-failure-and-recovery-candidate-2026-09-27).
Działanie tej wersji testowej w samochodzie wymaga jeszcze potwierdzenia.

### Diagnostyka zgodna z ELM

W `USB mode` wybierz szkic ELM327, wykonaj dwuklik i przytrzymaj, aby zapisać.
Następnie połącz zgodny program
diagnostyczny z nowym portem szeregowym C1. Oprogramowanie obsługuje ograniczony
zestaw funkcji CAN w stylu ELM. Nie jest pełnym ELM327 obsługującym wszystkie
protokoły. Nieobsługiwane polecenie może zwrócić `?`, a niedostępne dane —
`NO DATA`.

Podczas diagnostyki ELM zwykłe funkcje samochodowe BACCAble są wstrzymane,
więc nawigacja po menu może nie działać. Po odłączeniu USB sesja wygasa
po około dziesięciu sekundach. Kończy się też po 120 sekundach bez pełnego
polecenia ELM. Obsługiwane polecenia i działanie magistral opisuje
[dokumentacja diagnostyki USB](../docs/architecture/USB_DIAGNOSTICS.md).
Po zakończeniu używaj USB mode OFF do zwykłej pracy.

## Rozwiązywanie problemów

| Objaw | Co sprawdzić lub zrobić |
| --- | --- |
| RES nie otwiera menu | Włączony zapłon i aktywny zestaw wskaźników; CC i ACC wyłączone. Puść przyciski, a potem przytrzymaj RES przez 0,9 s. Sprawdź połączenie i oprogramowanie C1. Diagnostyka ELM może chwilowo wstrzymywać obsługę menu. |
| Menu przeskakuje albo przestaje reagować | Rozróżniaj lekkie i mocne naciśnięcia. Po przerwie w komunikacji puść przycisk. W Information sprawdź Reports/Gaps/Input age oraz stan C2/BH. |
| Na wyświetlaczu zostaje tekst radia | BH powinien ponawiać cały dwuliniowy komunikat BACCAble mniej więcej raz na sekundę i zaczynać go od początku po wiadomości radia. Używa źródła ostatniego odebranego tekstu multimediów. Jeśli menu nadal nie widać, sprawdź łącze BH, zgodność firmware i ramki nadawane przez BH; capture na komputerze zapisuje ramki odebrane, nie własną transmisję BH. |
| Brakuje strony lub akcji | Sprawdź profil silnika, Shown pages, Advanced pages oraz ustawienie udostępniające akcję w Features. Ukryta strona z innego profilu może nadal zajmować miejsce w ulubionych. |
| `No favorites` / `No pages` | Dodaj zgodne ulubione lub przywróć widoczność stron. Dwuklik RES nadal pozwala wrócić. |
| Odczyt pokazuje `--` | Poczekaj na aktualne dane, sprawdź stan zapłonu/silnika i obsługę przez ECU. Diagnostyka USB może wstrzymywać zwykłe zapytania. Obecność strony nie gwarantuje, że sterownik obsługuje jej parametr. |
| `Batt BCM SOC --` lub `Batt charge --%` | Porównaj niezależne źródła na Battery sources. Nie traktuj braku SOC jak 0% i nie używaj IBS override jako naprawy odczytu. |
| Ustawienie wraca do poprzedniej wartości po restarcie | Przytrzymaj RES w edytorze, aby zapisać; wcześniejszy dwuklik jest opcjonalny. Rozwiąż problem `× Save failed: RES`. Zmiany pozostawione tylko w pamięci roboczej nie są trwałe. |
| `! Disabled in setup` | Włącz odpowiednie ustawienie w Features i wróć. |
| `! Stop the car`, `! Enable Dyno`, `! Release brake`, `! Reset ESC`, `! Disable launch` | Spełnij wskazany warunek: zatrzymaj auto, włącz Dyno, zwolnij wymuszone hamowanie, przywróć normalny stan ESC albo wyłącz wspomaganie startu. Przy hamulcach stosuj opisaną procedurę wyłączenia wspomagania i zwolnienia. |
| `! Request pending` | Poczekaj na wynik oczekującego polecenia. Nie zakładaj, że już wykonano je albo anulowano. |
| `? C2 no reply` / `? BH no reply` | Sprawdź zasilanie, zgodność oprogramowania i połączenie między kontrolerami. Brak aktualnej odpowiedzi z wersją nie dowodzi zatrzymania całej magistrali CAN. |
| Lusterko nie opuszcza się | Sprawdź obsługę informacji o pozycji, zapis celu, Enabled, pracujący silnik, wsteczny i kierunkowskaz po właściwej stronie. Sprawdź też BH. Samo `Store: queued` nie potwierdza zapisu. |
| Pedal mode się zmienia, ale reakcja pedału nie | Sprawdź zgodność kontrolera pedału i komunikację. Porównaj odczyt Pedal map z wybranym ustawieniem. |
| Brak portu USB CAN | Wyjdź z DFU, uruchom zgodne oprogramowanie aplikacji, podłącz przewód danych i zapisz CAN dwuklikiem i przytrzymaniem przy pytaniu o zapis. Sprawdź, czy nie upłynął czas oczekiwania. Działające DFU potwierdza tylko połączenie USB bootloadera. |
| Pojawiają się tylko dwa z trzech portów | Ustal brakujący kontroler po nazwie produktu/numerze seryjnym albo podłączając pojedynczo. Sprawdź jego przewód, oprogramowanie i limit czasu trybu CAN. Ponownie zastosuj tryb z podłączonymi wszystkimi potrzebnymi przewodami. |
| Ramki się zapisują, ale sygnały są nieznane | Przechwytywanie nie nazywa automatycznie sygnałów. Porównuj zmiany ramek z zapisanymi czynnościami i powtarzanymi próbami. Zachowaj oryginalną sesję. |

## Różnice względem starej instrukcji

| Starsza nazwa lub sposób działania | Obecny odpowiednik |
| --- | --- |
| Krótkie RES wraca do menu głównego | Krótkie RES wybiera; **dwuklik RES cofa**. |
| Mocne naciśnięcie pomija dziesięć parametrów | Mocne naciśnięcie zmienia grupę; w Favorites przesuwa o jedną stronę. |
| Save & Exit | Szkic, potwierdzenie i przytrzymanie, aby zapisać; jawne ponowienie po błędzie. |
| Jedna płaska lista parametrów i funkcji | Favorites, Readings, Actions, Settings i Information. |
| Włączenie Park Mirror zapisuje pozycje | `Park mirror → Enabled` i `Store position` to osobne operacje. |
| Ogólne Battery % / BATTERY | Oddzielne źródła SOC: BCM, sterownik silnika i IBS. Zobacz tabelę źródeł akumulatora. |
| OIL UN. AIR | `MultiAir temp`, tylko I4. |
| SPARKL.1–4 | `Ignition cyl 1`–`Ignition cyl 4`; wartości oznaczają korekty zapłonu. |
| T-ON / OVER RPM | `Engine run time`, `Over-rev time`, `Over-rev count`. |
| ODOMETER LAST | `Distance (ECU)`. |
| EXAUST GAS / CATAL. / WATER | `Exhaust temp`, `Catalyst temp`, `Coolant temp`. |
| AIR COND. / CUR. GEAR / R-DNA | `A/C pressure`, `Gear`, `DNA mode`. |
| GO podczas pomiaru przyspieszenia | `RUN`; ukończony wynik jest w sekundach, nieudana próba pokazuje MISS. |
| Cykl hamulców Normal/Assist/Forced | Osobne akcje `Brake override` i `Disable launch`, z warunkami i potwierdzeniami. |
| Odczyt błędów opisany jako niedokończony | Działa odczyt BCM o ograniczonym zakresie. Nie jest skanerem wszystkich sterowników. |
| Immobilizer wśród funkcji menu | Stan tylko do odczytu w Information; osobny skrót właściciela pozostaje. |
| Zdalne uruchamianie silnika | W tym wydaniu nie ma dostępnej użytkownikowi, działającej opcji menu. Starszy opis nie potwierdza obecnej obsługi. |
| Wgranie CANable, aby przechwytywać CAN | `USB mode: CAN` udostępnia binarne przechwytywanie podczas pracy zgodnego zestawu C1/C2/BH. Osobne oprogramowanie CAN/SLCAN pozostaje innym sposobem użycia. |

## Źródła i zakres sprawdzenia

Polską wersję przygotowano na podstawie
[aktualnej instrukcji angielskiej](BACCAble_USER_GUIDE_EN.md).
Opisy w niej uzgodniono ze starszą instrukcją angielską DOCX/PDF oraz kodem:
[nawigacji i akcji menu](../firmware/baccable/features/menu.c),
[obsługi przycisków](../firmware/baccable/features/menu_input.c),
[definicji ustawień](../firmware/baccable/settings/setup_entries.c),
[kolejności i edycji ustawień](../firmware/baccable/settings/setup_menu.c),
[katalogu odczytów](../firmware/baccable/diagnostics/parameter_catalog.c),
[obsługi funkcji samochodu](../firmware/baccable/vehicle) oraz
[trybów USB](../firmware/baccable/features/usb_modes.c).

Dodatkowe materiały techniczne to
[opis menu](../docs/architecture/MENU_UX.md),
[przegląd katalogu odczytów](../docs/architecture/CATALOG_AUDIT.md) i
[plan prac](../docs/ACTION_PLAN.md). Część przykładów w starszych dokumentach
technicznych nadal używa dawnych nazw. Nazwy i kolejność w tej instrukcji
odpowiadają sprawdzonemu kodowi beta-13. Sprawdzenie dokumentacji nie oznacza
potwierdzenia działania każdej funkcji na rzeczywistym samochodzie, z każdą
wersją sterownika i każdym dodatkowym urządzeniem.
