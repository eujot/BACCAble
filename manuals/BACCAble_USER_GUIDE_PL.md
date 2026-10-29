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
- [Katalog odczytów](#katalog-odczytów)
- [Przechwytywanie CAN i diagnostyka przez USB](#przechwytywanie-can-i-diagnostyka-przez-usb)
- [Rozwiązywanie problemów](#rozwiązywanie-problemów)
- [Różnice względem starej instrukcji](#różnice-względem-starej-instrukcji)
- [Źródła i zakres sprawdzenia](#źródła-i-zakres-sprawdzenia)

## Pierwsze użycie

1. Zatrzymaj samochód i włącz zapłon. Uruchom silnik, jeśli wymaga tego funkcja
   lub odczyt. Część danych jest dostępna tylko przy pracującym silniku.
2. Wyłącz **CC i ACC**, a następnie puść przyciski kierownicy.
3. Przytrzymaj **RES przez około 0,9 sekundy** i puść. Otworzą się Favorites.
4. **Naciśnij RES dwukrotnie**, aby wrócić do menu głównego, i otwórz `Vehicle`.
5. Ustaw `Model`, `Model year`, `Engine`, `Drivetrain`, `Display` i wyposażenie.
   Pierwsze przytrzymanie otwiera podgląd skutków; góra/dół pokazuje ustawienia,
   których działanie zostanie wyłączone. Kolejne przytrzymanie zapisuje profil.
   BACK z edycji pola wraca do przeglądania; dwa BACK z przeglądania odrzucają cały
   szkic. Błąd zapisu pozostawia aktywny poprzedni profil i zachowuje szkic.
6. W `Display` wybierz `Auto`, `Legacy 18`, `Legacy 24` albo `MY23`. Zaktualizuj
   razem C1/C2/BH. `Auto` wybiera MY23 dla podanego roku 2023 lub nowszego.

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

Edytor Favorites ma takie same szkice, potwierdzenie, zapis przytrzymaniem
i odrzucanie. Zapis zostawia Cię w edytorze; osobny dwuklik wraca do Settings.
Bezczynność odrzuca niezapisane zmiany. Wybrany odczyt i ulubione są zapisywane
przy wyjściu ze zwykłych widoków. Przy `× Save failed: RES` krótkie RES lub przytrzymanie ponawia
zapis i wyjście, a dwuklik anuluje próbę wyjścia i zostawia bieżący widok.
Samo czekanie nie ponawia zapisu.

## Układ menu

```text
Menu główne
├─ Favorites                6 zestawów, 2 wiersze × maks. 4 pozycje
├─ Readings
│  ├─ Oil
│  ├─ Temperatures
│  ├─ Engine
│  ├─ Fuel
│  ├─ Battery
│  ├─ Diesel / DPF
│  ├─ Driving
│  ├─ Performance
│  └─ Diagnostics
├─ Actions                  Uruchom funkcję lub operację diagnostyczną
├─ Settings
│  ├─ Features               Ustawienia urządzenia i funkcji pojazdu
│  │  └─ Park mirror
│  │     ├─ Enabled
│  │     ├─ Store position
│  │     └─ < Back
│  ├─ Favorites              Favorite 1–6 → Row A/B → pozycja → odczyt + format
│  │  └─ Restore defaults    Potwierdzane przywrócenie tylko układów Favorites
│  └─ BACCAble IPC           Tymczasowe eksperymenty z odświeżaniem
├─ Information               Wersje, stan wejścia i Demo
└─ Vehicle                   Model, rok, silnik, napęd, ekran i wyposażenie
```

Kolejność menu głównego jest stała. Readings pokazuje pojedyncze pomiary
pogrupowane jak wyżej; grupy bez odczytu dostępnego dla skonfigurowanego profilu
pojazdu są pomijane. Dostępność odczytów zależy od profilu pojazdu (klasa silnika
i wyposażenie), a nie od starych przełączników `Advanced pages`/`Shown pages`.
Favorites składają pojedyncze odczyty w dwa logiczne wiersze. Przy pierwszym
uruchomieniu ładowane są domyślne zestawy zależne od silnika i zapisywane raz
(rekord UI 0x106).

## Profil pojazdu

`Vehicle` jest ostatnią pozycją menu głównego. `Engine` ma opcje `Unknown`,
`2.0 Petrol`, `2.9 Petrol`, `2.2 Diesel`; `Drivetrain`: `Unknown`, `RWD`, `Q4`.
`Model` to `Unknown`, `Giulia` albo `Stelvio`; rok to `Unknown` lub 2000–2099.
Pola wyposażenia mają wartości `Unknown`, `Absent`, `Present`: `Suspension`,
`Race`, `Exhaust valves`, `ACC`, `HAS`, `Lane assist`, `Park mirrors`, `SCR`.
Włączenie ustawienia funkcji nie potwierdza obecności wyposażenia.

Przeniesiona starsza instalacja zachowuje wcześniejsze uprawnienia do jawnego
zapisu Vehicle. Przed HOLD przejrzyj góra/dół pozycje `OFF: …`: ustawienia pozostają
zapisane, lecz niezgodne akcje i funkcje automatyczne nie będą wykonywane.
Nieznane wyposażenie w nowym lub potwierdzonym profilu nie zezwala na rozpoczęcie
akcji, która go wymaga. Race i regulowane zawieszenie są niezależne. Żądanie AWD
wymaga Q4. Wyposażenie oznaczone jako nieobecne ukrywa odpowiednią opcję funkcji;
`Configure Vehicle` wyjaśnia nieznany warunek. Zatrzymanie lub zwolnienie już
rozpoczętej operacji pozostaje dostępne.

Przed zapisem profilu zakończ operacje AWD, hamulców, Dyno, wydechu i lusterek
parkingowych; edytor wskazuje operację do zakończenia. Stan lusterek musi być
aktualny. Zapis nie ponawia startowych żądań DNA ani zawieszenia. `Saved` pojawia
się po poprawnym atomowym zapisie; `Save failed` pozostawia poprzedni profil
aktywny i pozwala ponowić próbę. Własne Favorites są zachowane przy zmianie profilu.

`Auto` wybiera MY23 dla podanego roku >=2023, w pozostałych przypadkach Legacy 18.
Jawny wybór ekranu ma pierwszeństwo przed rokiem. Nowa szerokość działa po
potwierdzeniu obsługi protokołu przez BH; `Saved; IPC pending` oznacza zapisany
profil i oczekiwanie przy zachowanym poprzednim działającym przesyłaniu obrazu.
Po uruchomieniu legacy korzysta z szerokości kompilacji do zakończenia uzgodnienia.
Zaktualizuj wszystkie płytki razem. Próby rzeczywistego ekranu pozostają otwarte.

## Symbole i komunikaty

Starszy profil, zgodnie z wyborem `Display`, wykorzystuje 18 lub 24
pozycje tekstowe. Nazwa na liście może różnić się od podpisu bieżącego odczytu,
aby zrobić miejsce na wartości. Na przykład strona `Batt charge BCM` wyświetla
odczyt podpisany `Batt BCM SOC`.

MY23 ma **16 widocznych znaków w pierwszej linii i 22 w drugiej**. Lista
pokazuje bieżącą pozycję z `›`, a pod nią następną. Edytor pokazuje ustawienie
oraz szkic wartości z `HOLD SAVE`. Druga linia zawiera również pytanie o zapis
lub jego wynik. Ulubione pokazują po jednym wierszu zestawu w każdej linii. Readings pokazuje
nazwę jednego pomiaru w pierwszej linii, a jego wartość w drugiej.
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

Favorites to sześć zestawów, każdy z dwoma wierszami (**A** i **B**) po maksymalnie
cztery pozycje. Każda pozycja przechowuje jeden pojedynczy odczyt i format
wyświetlania: **Auto**, **Full**, **Medium** lub **Short**. **Auto** wybiera raz
najczytelniejszą mieszczącą się etykietę. Wiersz ulubionych łączy pozycje co
najmniej jedną spacją i nigdy nie obcina cyfr; `--` oznacza brak lub nieaktualność.
MY23 pokazuje oba wiersze; legacy przechodzi po niepustych wierszach (1/A, 1/B,
2/A, …). Ten sam odczyt może wystąpić w kilku widocznych wierszach i jest odpytywany tylko raz.
Legacy odpytuje tylko widoczny wiersz lub podstronę; MY23 widoczne pozycje obu
wierszy. Szerszy zapisany wiersz na węższym ekranie jest dzielony na stałe podstrony,
bez zmiany zapisanego układu. Pojedyncza zapisana etykieta zbyt szeroka dla nowego
ekranu tymczasowo używa skrótów Auto. Pozycje nie przesuwają się przy zmianach wartości.
Nieoczekiwane przekroczenie miejsca na liczbę pokazuje `#` w polu tej wartości.

### Domyślne zestawy ulubionych

Pierwsze uruchomienie ładuje domyślne zestawy zależne od silnika i zapisuje je raz.
Późniejsza zmiana profilu pojazdu nigdy nie nadpisuje zapisanego układu;
nieobsługiwana zapisana pozycja jest ukryta i nie jest odpytywana, dopóki znów
nie pasuje do profilu. Jej zapisane miejsce i format pozostają zachowane.

| Zestaw | Wiersz A | Wiersz B |
| --- | --- | --- |
| Warm-up | Oil temp + coolant | Gearbox temp |
| Oil | Oil level mm + LOW/OK/HIGH | Oil level bar |
| Engine | Power + boost | Power + torque |
| Cooling | Coolant | Intercooler inlet + outlet |
| Battery | Voltage + current | IBS state of charge |
| Diesel DPF / Gasoline Performance | DPF load + temp (diesel) / 0–100 (benzyna) | Regen progress (diesel) / Best 0–100 (benzyna) |

Domyślne zestawy MY23 upraszczają pierwszą linię do 16 znaków: Warm-up pokazuje
temperaturę oleju nad temperaturą skrzyni + płynu chłodzącego; Oil poziom liczbowy
nad paskiem; Engine doładowanie nad mocą + momentem; Battery napięcie nad stanem
naładowania IBS + prądem. Diesel DPF pokazuje zapełnienie nad postępem + temperaturą.
Cooling i benzynowy Performance zachowują układ z tabeli. Legacy zachowuje tabelę,
a szerokie wiersze dzieli na podstrony.

### Edycja ulubionych

1. Otwórz `Settings → Favorites` i wybierz `Favorite 1`–`Favorite 6`.
2. Wybierz `Row A` lub `Row B`, a następnie pozycję.
3. Wybierz odczyt (albo `Empty`) i format (Auto/Full/Medium/Short).
4. Przytrzymaj, aby zapisać. Pierwszy BACK otwiera pytanie o zapis zmienionego
   szkicu, a drugi go odrzuca. `Duplicate reading`, `Oil bar needs row` lub
   `Too wide: shorten` odrzuca zapis i zachowuje szkic. Wybierz krótszy format
   albo usuń pozycję.

`Settings → Favorites → Restore defaults` znajduje się za `Favorite 6`.
SELECT otwiera potwierdzenie, HOLD zastępuje wszystkie sześć zestawów, BACK anuluje.
Błąd zapisu zachowuje poprzednie układy. Ustawienia, profil, statystyki i kalibracja
lusterek pozostają zachowane.

### Poziom oleju i pasek

`Oil level` (mm + LOW/OK/HIGH) i `Oil level bar` to dwa osobne odczyty; pasek
zajmuje cały wiersz sam. Zakresy OK podane przez właściciela są bez zmian: 2.0 RWD
72–82 mm, 2.0 Q4 67–77 mm, 2.2 RWD 61–65 mm, 2.2 Q4 79–84 mm. Nie są one tutaj
na nowo weryfikowane. Nieznany napęd pokazuje `?` i `Set drivetrain`.

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
| `DPF regen alert` | Wyłączona / włączona | Włącza powiadomienia o regeneracji DPF w dieslu. Zobacz [obserwowanie DPF](#obserwowanie-dpf). |
| `Auto rotate` | Wyłączona / włączona | Automatycznie zmienia stronę bieżących odczytów co pięć sekund. |

### Grupa komfortu

| Opcja | Wartości | Do czego służy |
| --- | --- | --- |
| `Seatbelt alarm` | Wyłączona / włączona | Wysyła polecenie zmiany ustawienia sygnału niezapiętych pasów. To inna pozycja niż odczyt Seatbelt alarm. Sprawdź wynik w samochodzie; samo odłączenie BACCAble nie musi przywrócić poprzedniego ustawienia sterownika. |
| `Stop odo blink` | Wyłączona / włączona | Powstrzymuje miganie licznika przebiegu. Nie usuwa niezgodności konfiguracji, która spowodowała miganie. |
| `Park mirror` | Podmenu | Włącza ustawianie lusterek do cofania i pozwala osobno zapisać ich pozycje. Zobacz [procedurę ustawiania lusterek](#lusterka-podczas-cofania). |
| `Close windows` | OFF / 1 lock / 2 locks | Wybiera zamykanie szyb po jednym lub dwóch naciśnięciach zamykania na pilocie. OFF wyłącza funkcję. Krótsza nazwa na wyświetlaczu to `Close win`. |
| `Open windows` | OFF / 1 unlock / 2 unlocks | Wybiera otwieranie szyb po jednym lub dwóch naciśnięciach otwierania na pilocie. OFF wyłącza funkcję. Krótsza nazwa to `Open win`. |
| `QV exhaust` | Wyłączona / włączona | Udostępnia sterowanie klapami wydechu przez akcję w menu i podwójne naciśnięcie przycisku odblokowania dźwigni zmiany biegów. Wymaga zgodnego sprzętu. |
| `Rain lights` | OFF / ON; domyślnie OFF | W AUTO prosi płytkę BH o zgłoszenie ciemności po trzech wykrytych cyklach wycieraczek w ciągu 15 sekund. Podtrzymuje żądanie przez 60 sekund od ostatniego cyklu. Wymaga zgodnego firmware C1/C2/BH. Działanie lamp wymaga jeszcze potwierdzenia w samochodzie. |
| `Auto PDC mute` | Wyłączona / włączona | Chwilowo wycisza aktywny sygnał czujników parkowania podczas hamowania poza biegiem wstecznym. Przywraca działanie zmienione przez BACCAble po odpuszczeniu hamulca lub wybraniu wstecznego. Wymaga aktualnych danych o hamulcu, biegu i czujnikach PDC. |
| `Mute audio in R` | Wyłączona / włączona | Wycisza dźwięk systemu audio na wstecznym i przywraca go po zakończeniu cofania, jeśli to BACCAble go wyciszyło. Uwzględnia wcześniejsze wyciszenie oraz ręczne przywrócenie dźwięku. Działa niezależnie od wyciszania PDC. |


`Settings` → `Features` → `Comfort` → `Rain lights`: wybierz ON i przytrzymaj przycisk, aby zapisać.
Czasy są stałe. Każdy kolejny wykryty cykl rozpoczyna od nowa 60 sekund
podtrzymania. Jeden ruch nie uruchamia funkcji; kilka ruchów po użyciu
spryskiwacza może ją uruchomić. Funkcja reaguje na pracę wycieraczek, nie mierzy
intensywności deszczu. Bardzo rzadkie ruchy mogą nie osiągnąć progu.

Opuszczenie AUTO, wyłączenie opcji lub brak aktualnych danych albo łączności
kończy żądanie. C2 przesyła je bezpośrednio do BH; BH sprawdza też własny,
aktualny sygnał AUTO i wygasza żądanie po trzech sekundach bez ponowienia.
Po każdej świeżej ramce czujnika BH `0x738` BH kopiuje ją i nadaje dwa surowe
bajty światła jako zero, dopóki żądanie jest aktywne. Potem przestaje nadawać
ciemne kopie; nie wysyła polecenia gaszenia lamp. W zapisanych próbach fabryczne
AUTO gasiło lampy około 14–20 sekund po powrocie światła. Nie potwierdzono
jeszcze, czy samochód przyjmie dodatkową ramkę i włączy przednie oraz tylne
lampy. Sprawdź rzeczywiste lampy w dziennej próbie na postoju. Harmonogram
łączności nadal pozwala na odczyt statusu C2 i nagrywanie USB.
Przy włączonym `Rain lights` niezmieniony tekst menu jest ponawiany rzadziej,
aby zostawić miejsce na wspólnym łączu dla komend świateł i statusu. Zmiana
pozycji menu nadal pojawia się od razu.

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
| `Rain test` | Nie wymaga osobnego ustawienia; zgodny firmware C1/C2/BH | Otwiera dwie strony diagnostyki świateł deszczowych; góra/dół przełącza między wycieraczkami a odczytem sygnału światła z BH. W AUTO przycisk SELECT żąda ciemnych kopii BH `0x738` przez maksymalnie 30 s; BH kończy wymuszenie po 3 s bez ponowienia. BACK kończy test. |

Pierwsza strona `Rain test` działa jak dotąd: `Wiper` liczy zbocza ruchu
wycieraczek także przy wyłączonym `Rain lights`; pierwsza próbka po przerwie
ustala tylko stan początkowy. `A` pokazuje aktualny stan AUTO (`?` oznacza brak
danych), `ARM` aktywne żądanie świateł, `Q` liczbę aktywnych żądań C2→BH
przyjętych do kolejki UART modulo 256, a `F` żądanie wymuszenia z C1.
`C2: no reply` oznacza brak świeżych danych z C2. `Q` nie potwierdza nadania
ramki CAN przez BH ani zapalenia lamp.
Druga strona pokazuje na przykład `BH738 6/6`: dwie oddzielne, **surowe**
wartości z bajtów 1 i 2 ramki BH `0x738`. Podczas zapisanych testów w świetle
dziennym zwykle było `6/6`, a po zasłonięciu czujnika `0/0`; nie są to luksy ani
skalibrowane progi jasności. `TX` w drugim wierszu liczy ciemne ramki `0x738`
przyjęte do kolejki CAN płytki BH modulo 256; nie potwierdza przyjęcia ich przez
samochód. `BH738 no reply` oznacza brak aktualnej odpowiedzi BH, a `BH738 no
frame` — odpowiedź BH bez świeżej ramki `0x738`. Widoczna
strona odświeża dane mniej więcej co dwie sekundy. Podczas wymuszenia C2 jest
odpytywane co sekundę, aby podtrzymać żądanie, a odczyt BH aktualizuje się mniej
więcej co pięć sekund. `BH738 no reply` z komunikatem `UART/BH timeout` oznacza
brak świeżej odpowiedzi; nie dowodzi, że płytka BH jest odłączona.

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

Wybierz `Readings`, grupę i odczyt. Podpis przy bieżącej wartości może być krótszy
niż jego etykieta. Wszystkie odczyty opisane poniżej są w katalogu, ale
nieobsługiwane zapytania do sterownika ECU mogą stale dawać `--`.

- **Power** to moc obliczona z podawanego momentu i obrotów silnika. Jednostka
  `PS` to metryczne konie mechaniczne, czyli polskie KM; w starej instrukcji
  używano oznaczenia CV. To nie jest pomiar z hamowni.
- **Objętość, poziom, jakość, ciśnienie i temperatura oleju to różne parametry.**
  Jakość jest oceną sterownika, a nie wynikiem badania laboratoryjnego.
  Poziom oleju oceniaj w warunkach pomiaru zalecanych przez producenta pojazdu.
- **Ignition cyl 1–6** pokazuje korektę zapłonu dla danego cylindra w stopniach.
  Nie jest licznikiem wypadania zapłonów. Dla V6 wypadanie zapłonów obejmuje
  tylko cylindry 1–4, ponieważ katalog nie zawiera potwierdzonych definicji
  indywidualnych liczników dla cylindrów 5 i 6.
- **Distance (ECU)** to odczyt diagnostyczny opisany w starej instrukcji jako
  dystans od wyzerowania licznika ECU. Bez sprawdzenia interpretacji danego
  sterownika nie utożsamiaj go z całkowitym przebiegiem na zestawie wskaźników.
- **Gear** pokazuje N, R lub numer biegu; DNA pokazuje A, N, D lub R.
  Brak rozpoznanego stanu może dawać `--`/`?`. `Pedal map` korzysta z odpowiedzi
  zewnętrznego kontrolera pedału.

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

## Katalog odczytów

Readings to pojedyncze pomiary pogrupowane jak poniżej; grupy bez odczytu
dostępnego dla skonfigurowanego profilu pojazdu są pomijane. Ekran może używać krótszej etykiety.
Wartość, na którą ECU nie odpowiada, pokazuje `--`.

### Oil

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Oil temp | C | wszystkie |
| Oil pressure | bar | wszystkie |
| Oil level | mm | wszystkie |
| Oil status | LOW / OK / HIGH | wszystkie |
| Oil bar | pasek na całą szerokość | wszystkie |
| Oil volume | L | wszystkie |
| Oil quality | % | wszystkie |

### Temperatures

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Coolant | C | wszystkie |
| Gearbox | C | wszystkie |
| IC inlet | C | benzyna |
| IC outlet | C | benzyna |
| Intake air | C | diesel |
| Exhaust | C | wszystkie |
| Fuel temp | C | diesel |
| Catalyst | C | benzyna |

### Engine

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Power | PS | wszystkie |
| Torque | Nm | wszystkie |
| Boost | bar | wszystkie |
| Intake | bar | benzyna |
| Ign cyl 1 | deg | wszystkie |
| Ign cyl 2 | deg | wszystkie |
| Ign cyl 3 | deg | wszystkie |
| Ign cyl 4 | deg | wszystkie |
| Ign cyl 5 | deg | benzyna 2.9 |
| Ign cyl 6 | deg | benzyna 2.9 |
| Misfires | liczba | wszystkie |
| EGR target | % | diesel |
| EGR actual | % | diesel |
| Turbo temp | C | diesel |

### Fuel

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Fuel rate | L/h | diesel |
| Fuel rail | bar | diesel |

### Battery

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Battery | V | wszystkie |
| Battery A | A | wszystkie |
| Batt IBS | % | wszystkie |
| Batt ECU | % | wszystkie |
| Batt BCM | % | wszystkie |
| IBS raw 0 | surowy bajt | wszystkie |
| IBS raw 1 | surowy bajt | wszystkie |

### Diesel / DPF

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| DPF load | % | diesel |
| DPF temp | C | diesel |
| Regen | % | diesel |
| Regen mode | faza | diesel |
| Regen ago | km | diesel |
| Regen count | liczba | diesel |
| Regen avg km | km | diesel |
| Regen avg min | min | diesel |
| AdBlue | L | diesel + SCR |
| AdBlue % | % | diesel + SCR |

### Driving

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Speed | km/h | wszystkie |
| Gear | N / R / bieg | wszystkie |
| DNA mode | A / N / D / R | wszystkie |
| Pedal map | litera mapy | wszystkie |
| Seatbelt | ON / OFF | wszystkie |
| Distance | km | wszystkie |

### Performance

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| 0-100 | s | wszystkie |
| 100-200 | s | wszystkie |
| Best 0-100 | s | wszystkie |
| Best100-200 | s | wszystkie |

### Diagnostics

| Odczyt | Jednostka / znaczenie | Dostępność |
| --- | --- | --- |
| Oil ECU temp | C | wszystkie |
| Oil ECU press | bar | wszystkie |
| MultiAir | C | benzyna 2.0 |
| Knock | mV | benzyna |
| Key ID | liczba | benzyna |
| Run time | min | wszystkie |
| Over-rev s | s | wszystkie |
| Over-rev n | liczba | wszystkie |
| A/C pressure | bar | wszystkie |
| Turbo V | V | benzyna |
| Intake V | V | diesel |
| Turbo req bar | bar | diesel |
| Turbo req % | % | diesel |
| Turbo % | % | diesel |
| Boost req | bar | diesel |

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
| Brakuje odczytu lub akcji | Sprawdź profil pojazdu oraz ustawienie udostępniające akcję w Features. Nieobsługiwane zapisane ulubione pokazuje `--`, dopóki jego profil znów nie zacznie obowiązywać. |
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

Readings i Favorites to teraz pojedyncze pomiary. Stare strony łączone,
`Page favorites`, `Shown pages`, `Favorite order` i `Sort order` zniknęły;
Favorites to sześć dwuwierszowych układów, a Readings to pogrupowane pojedyncze
odczyty (patrz [Układ menu](#układ-menu)). Stare identyfikatory stron celowo nie
są zachowywane; pierwsze uruchomienie ładuje i zapisuje raz nowe domyślne zestawy.

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
