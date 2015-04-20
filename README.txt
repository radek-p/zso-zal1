┌─────────────────────────────────┐
│ autor:      Radosław Piórkowski │
│ nr indeksu: 335451              │
└─────────────────────────────────┘

Na rozwiązanie składają się pliki:
  ╠═ loader.h
  ╠═ loader_private.h
  ╠═ debug.h
  ╠═ loader.c
  ╚═ lazy-resolve.s
  
  Kompilacja
  ──────────
  
  Użyty został system budowania CMake. Aby wygenerować obiekt współdzielony
  libloader.so, można wykonać skrypt ./build-loader.sh, który wykonuje proste
  komendy Cmake'a.  
  
  Plik libloader.so będzie się znajdował w katalogu build/.
  
  Aby uruchomić testy, które zostały udostępnione na stronie zadania, można wykonać:
  
  ./run-tests.sh
  
  Ogólny opis rozwiązania
  ───────────────────────
  
  - nagłówek pliku Elf oraz tabela nagłówków programu są ładowane do pamięci
    przy pomocy funkcji fread,
    
  - segmenty PT_LOAD są mapowane do pamięci w 3 krokach:
      a) wykonywany jest "duży" mmap pokrywający odpowiedni rozmiar,
      b) poszczególne segmenty są mapowane z pliku,
      c) po wykonaniu relokacji ustawiane są odpowienie uprawnienia przy pomocy mprotect.
      
  - rozwiązanie obsługuje leniwe szukanie symboli:
      Kod pozwalający przeprowadzić wyszukiwanie symboli i zachowujący
      przy tym odpowienie rejestry znajduje się w pliku lazy-resolve.s.
      
  - funkcja library_load przy niepowodzeniu wszelkiego rodzaju:
      a) wypisuje komunikat na standardowe wyjście błędu,
      b) ustawia zmienną errno na EINVAL,
      c) zwraca NULL.
      
  - komunikaty diagnostyczne (całkiem obszerne) można włączyć, odkomentowując
    odpowiedni wpis w CMakeLists.txt
      
  - podczas leniwego wyszukiwania symbolu niepowodzenie powoduje:
      a) wypisanie błędu na standardowe wyjście błędu,
      b) exit(1)
      
  - w treści zadanie powiedziano, iż biblioteka nie ma eksportować innych symboli, niż
    dwie funkcje zdefiniowane w loader.h.
      Aby to zapewnić, użyto komendy GCC "#pragma GCC visibility push(hidden)"
      (plik loader_private.h)
      

