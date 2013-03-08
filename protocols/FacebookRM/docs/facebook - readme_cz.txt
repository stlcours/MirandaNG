--------------------------------
| Facebook Protocol RM 0.0.9.5 |
|        pro Miranda NG        |
|          (2.3.2013)          |
--------------------------------

Autor: Robyer
  E-mail: robyer@seznam.cz
  Jabber: robyer@jabbim.cz
  ICQ: 372317536
  Web: http://www.robyer.cz
 
Info: 
 - Tento plugin je zalo�en na Facebook Protokolu (autor jarvis) verze 0.1.3.3 (open source).
 - Jeho verze je k nalezen� na: http://code.google.com/p/eternityplugins/

--------------------------------
      Informace o stavech
--------------------------------
 - Online = p�ipojen k fb, chat je online
 - Neviditeln� = p�ipojen k fb, ale pouze pro z�sk�v�n� novinek a nov�ch ozn�men� - CHAT je OFFLINE
 - Offline = odpojeno

--------------------------------
       Skryt� nastaven�
--------------------------------
"TimeoutsLimit" (Byte) - Errors limit (default 3) after which fb disconnects
"DisableLogout" (Byte) - Disables logout procedure, default 0
"PollRate" (Byte) - Waiting time between buddy list and newsfeed parsing.

--------------------------------
       Historie verz�
--------------------------------

... TODO ...

=== OLD CHANGES (MIRANDA IM) ===

0.0.9.3 - 12.12.2012
 ! Opraven p�d kdy� nebyl na str�nce nalezen vlastn� avatar
 * Plugin linkov�n staticky

0.0.9.2 - 23.10.2012
 ! Opraveny zm�ny avatar� kontakt�
 ! Opraveno p�ipojen� s jedn�m typem vy��d�n� nastaven� n�zvu po��ta�e
 ! P�epracov�no po�adavk� pro odes�l�n� zpr�v (m�lo by zamezit vz�cn�mu banu od FB :))
 ! V�choz� skupina pro nov� kontakty se vytvo�� p�i p�ihl�en�, pokud je�t� neexistuje
 ! Oprava zm�n viditelnosti z jin�ho klienta
 * Plugin linkov�n znovu proti C++ 2008 knihovn�m

0.0.9.1 - 16.10.2012
 ! Opraveno nefunk�n� p�ihl�en� kv�li zm�n� na Facebooku
 * Plugin linkov�n proti C++ 2010 knihovn�m

0.0.9.0 - 11.6.2012 
 + P�ij�m�n� po�adavk� o p��telstv� (kontrola ka�d�ch cca. 20 minut)
 + P�epracov�ny autorizace - ��d�n�, potvrzov�n�, ru�en� p��telstv�
 + Podpora vyhled�v�n� a p�id�v�n� lid�
 * Zm�na n�kter�ch �et�zc�
 * Pou�it� stejn�ho GUID pro 32bit a 64bit verzi
 ! P�ij�m�n� zpr�v s origin�ln�m �asov�m raz�tkem
 ! Opraveno maz�n� avatar� kontakt� se stavem "Na mobilu"
 ! Uvol�ov�n� OnModulesLoaded hooku (d�ky Awkward)
 ! Neodes�lat upozorn�n� na psan� kontakt�m, kte�� jsou offline 
 ! Oprava pr�ce s avatary (d�ky borkra)
 ! SetWindowLong -> SetWindowLongPtr (d�ky ghazan) 
 ! Funk�n� p�ihl�en� s pozvrzen�m posledn�ho pokusu o p�ihl�en� z nezn�m�ho za��zen�
 ! Opraveno odes�l�n� zpr�v do skupin
 ! R�zn� dal�� intern� opravy

0.0.8.1 - 26.4.2012
 ! Opraveno p�ij�m�n� ozn�men� po p�ipojen�
 ! Opraveno p�ij�m�n� nep�e�ten�ch zpr�v po p�ipojen�
 ! P�ij�m�n� nep�e�ten�ch zpr�v po p�ipojen� se spr�vn�m �asov�m raz�tkem
 ! Opraveno oznamov�n� novinek ze zdi
 ! Oprava souvisej�c� s maz�n�m kontakt�/ru�en�m p��telstv�
 + Nov� volba novinek ze zdi "Aplikace a hry"
 + Kontakty maj� nyn� MirVer "Facebook" 
 + U offline zpr�v se p�ij�maj� i p��padn� p��lohy
 ! Opraveno fungov�n� avatar� v Mirand� 0.10.0#3 a vy��� (d�ky borkra)
 ! N�jak� drobn� opravy (d�ky borkra)
 
 x Nefunguje ozn�men� o po�adavc�ch o p��telstv�

0.0.8.0 - 14.3.2012
 # Pro spu�t�n� pluginu je vy�adov�na Miranda verze 0.9.43 a nov�j��
 # Plugin je kompilov�n skrz VS2005 (Fb x86) a VS2010 (Fb x64)
 + P�id�ny 2 typy p��sp�vk� k oznamov�n� ze zdi: Fotky a Odkazy 
 * P�epracov�no nastaven�
 ! Opraveno nastaven� oznamov�n� jin�ho typu novinek ze zdi
 ! Opraveno a vylep�eno parsov�n� novinek ze zdi
 ! Opraveno p�ij�m�n� zpr�v v chatu, ve kter�ch je %
 ! Opraveno nefunk�n� p�ihl�en� 
 ! Vylep�eno maz�n� kontakt�  
 + Podpora EV_PROTO_ONCONTACTDELETED ud�lost� 
 + P�id�no chyb�j�c� GUID pro x64 verzi a aktualizov�n user-agent  
 ! N�jak� dal�� mal� opravy a vylep�en�
 ! Opravena polo�ka 'Visit Profile', pokud je v hlavn�m menu
 * Aktualizov�n language pack soubor (pro p�ekladatele)
 - Vypnuta mo�nost pro zav�r�n� oken na facebooku (do�asn� nefunguje)

0.0.7.0 - 19.1.2012
 + Podpora skupinov�ch chat� (EXPERIMENT�LN�!) - zapn�te v nastaven�
 ! Opraveno na��t�n� seznamu kontakt�
 ! Oprava mo�n�ho zamrznut� Mirandy

0.0.6.1 - 6.1.2012
 + Vr�cena mo�nost v nastaven�, pro zav�r�n� oken chatu (na webu)
 + Mo�nost mapovat nestandardn� stavy na stav Neviditeln� (m�sto na Online)
 + Mo�nost zobrazovat lidi, kte�� maj� stav "na mobilu"
 ! Opravena zm�na viditelnosti v chatu
 ! Velmi dlouh� zpr�vy u� se nep�ij�maj� duplicitn�
 ! �pravy ohledn� skupinov�ch zpr�v a p��jmu zpr�v od lid�, kte�� nejsou mezi p��teli

0.0.6.0 - 18.11.2011
 * P�epracov�no nastaven�
  + Mo�nost pou��t https p�ipojen� i pro "Channel" po�adavky
  + Mo�nost pou��vat v�t�� avatary kontakt�
	+ Mo�nost p�ij�mat nep�e�ten� zpr�vy p�i p�ihl�en� (EXPERIMENT�LN�!)
	+ Mo�nost p�i odpojen� protokolu odpojit tak� chat.
  - Zru�ena mo�nost nastaven� User-Agenta
  - Zru�ena mo�nost zobrazen� Cookies
 * Do datab�ze se ukl�d� datum a �as zji�t�n� odstran�n� kontaktu ze serveru (hodnota "Deleted" typu DWORD)
 + Mo�nost v menu kontaktu pro smaz�n� ze serveru.
 + Mo�nost v menu kontaktu pro po��d�n� o p�id�n� mezi p��tele.
 + P�i smaz�n� kontaktu z Mirandy se zobraz� dialog s mo�nost� smazat i ze serveru 
 ! Opraveno nefunk�n� p�ihl�en�

0.0.5.5 - 16.11.2011
 ! Opraveno neodpojov�n� kontakt�

0.0.5.4 - 16.11.2011 
 ! Opraveno n�kolik probl�m� s p�ipojen�m
 ! R�zn� opravy p�d�, �nik� pam�ti a komunikace (d�ky borkra)
 @ Pokud se v�m Facebook odpojuje a m�te zapnut� HTTPS p�ipojen�, vypn�te si volbu "Ov��it platnost certifik�t�" pro Facebook v Mo�nosti/S�t�.

0.0.5.3 - 31.10.2011
 ! Opraven probl�m s p�ihl�en�m pro n�kter� lidi.
 ! Opraveno nep�ij�m�n� n�kter�ch zpr�v
 ! Opraveno chybn� na��t�n� vlastn�ho jm�na
 * Rychlej�� odes�l�n� zpr�v

0.0.5.2 - 31.10.2011
 ! Opraven probl�m s p�ipojen�m a p�dem.

0.0.5.1 - 30.10.2011
 ! Work-around pro pos�l�n� zpr�v obsahuj�c� odkazy.

0.0.5.0 - 29.10.2011
 + Ozn�men� o p��tel�ch, kte�� se znovu objevili na serveru.
 * Kompletn� p�epracov�na podpora avatar�.
 * Ve v�choz�m nastaven� se pou��vaj� mal� �tvercov� avatary (lze zm�nit skryt�m nastaven�m "UseBigAvatars")
 ! Opraveno pou�it� skryt�ho nastaven� "UseBigAvatars" 
 ! Opraveno nastavov�n� stavov� zpr�vy.
 ! Opraven p�d p�i pou�it� MetaContacts.
 ! Opraveno p�ihl�en� pro n�kter� lidi.
 ! Opravy ohledn� ozn�men� ud�lost� p�i p�ipojen�.
 ! Opraven �nik pam�ti souvisej�c� s ozn�men�m.
 ! Opraveno zji��ov�n� nep�e�ten�ch zpr�v p�i p�ihl�en� (pokud je pou�ito skryt� nastaven� "ParseUnreadMessages")
 ! Opraveno p�ipojen� s nastaven�m N�zvu za��zen�. 
 ! R�zn� opravy, vylep�en� a o�ista k�du. 
 - Odstran�n skryt� kl�� "OldFeedsTimestamp" 
 @ D�ky borkra pro pomoc se spoustou v�c�. 

0.0.4.3 - 22.9.2011
 ! Oprava nov�ho layoutu facebooku
 ! Oprava na��t�n� jm�na kontaktu pro nov� kontakty

0.0.4.2 - 15.9.2011
 ! Opraveno neaktivov�n� polo�ek stavov�ho menu
 ! Nenastavov�n� automaticky kontaktu do "Online" stavu, kdy� od n�j p�ijde zpr�va
 ! Oprava k�dov�n� chybov�ch hl�ek p�i odes�l�n� zpr�vy
 + Podpora odes�l�n� zpr�v ve stavu "Neviditeln�" 
 + Stav protokolu respektuje zm�ny stavu chatu na str�nce
 + P�i p�ihl�en� ozn�men� konkr�tn�ch upozorn�n� nam�sto jejich pouh�ho po�tu
 + P�id�n skryt� kl�� (ParseUnreadMessages) pro p�ij�m�n� nep�e�ten�ch zpr�v p�i p�ihl�en� nam�sto ozn�men� jejich po�tu << POZOR: nen� pln� funk�n�!!!

0.0.4.1 - 13.9.2011
 ! Vr�ceny zm�ny zp�sobuj�c� neodpojov�n� kontakt�

0.0.4.0 - 12.9.2011
 * Intern� zm�ny ohledn� zm�ny stavu
 - Odstran�na podpora stavu "Na chv�li pry�"
 ! Opraveno parsov�n� novinek ze zdi
 - Zru�eno ozn�men� o po�tu nep�e�ten�ch zpr�v ve stavu "Neviditeln�"
 + Ve stavu "Neviditeln�" p�ij�m�n� zpr�v p��mo
 + Na��t�n� pohlav� kontakt�
 + Na��t�n� v�ech kontakt� ze serveru (tzn. nejen ty, co jsou pr�v� online)
 + Informov�n� o tom, �e n�kdo zru�il p��telstv� s v�mi (= nebo si jednodu�e deaktivoval facebook ��et)

0.0.3.3 - 17.6.2011
 ! Oprava spr�vn� komunikace (zji��ov�n� seq number)
 ! Oprava ozn�men� s nep�e�ten�mi ud�lostmi p�i p�ipojen�

0.0.3.2 - 8.6.2011
 ! Ozna�ov�n� zpr�v na facebooku jako p�e�ten�ch.
 * Vypnuto oznamov�n� o channel refresh

0.0.3.1 - 23.5.2011
 ! Opraveno upozorn�n� p�i neodesl�n� zpr�vy kv�li vypr�en� �asov�ho limitu

0.0.3.0 - 22.5.2011
 ! Opraveno na��t�n� avatar�
 ! Opraveno zobrazov�n� avatar� v SRMM
 ! Opraveno nena�ten� pluginu pro n�kter� lidi s Mirandou 0.9.17
 ! Opraveny p�dy a zamrz�v�n� p�i maz�n� ��tu
 ! N�kolik oprav a vylep�en� spojen�ch s p�ihla�ov�n�m
 ! Oprava ob�asn�ho chybn�ho parsov�n� novinek ze zdi 
 ! Oprava p�du p�i p�ipojen� pokud jsou nep�e�ten� ud�losti 
 ! Oprava n�kter�ch dal��ch p�d�
 * Vylep�eno z�sk�v�n� avatar� kontakt� (2x rychlej��)
 * Optimalizace nastaven� kompilace -> 2x men�� v�sledn� soubor (d�ky borkra)
 + Pou�ito persistentn� http p�ipojen� (d�ky borkra)
 + Ni�en� slu�eb a hook� p�i ukon�en� (d�ky FREAK_THEMIGHTY)
 + Podpora pro per-plugin-translation (pro MIM 0.10#2) (d�ky FREAK_THEMIGHTY)
 + Podpora pro EV_PROTO_ONCONTACTDELETED (pro MIM 0.10#2) (d�ky FREAK_THEMIGHTY)
 - Nep�ekl�d�n� n�zvu protokolu v nastaven�
 - Zru�ena "optimalizace" pro zav�r�n� oken. 
 ! Opraveno odes�l�n� informace o psan�
 ! Opraveno parsov�n� odkaz� u novinek na zdi
 * Povoleno odes�l�n� offline zpr�v
 ! Opraveno oznamov�n� chyb p�i odes�l�n� zpr�v (+ zobrazen� konkr�tn� chyby)
 + 5 pokus� o odesl�n� zpr�vy p�ed zobrazen�m chybov� hl�ky.

0.0.2.4 - 6.3.2011
 ! Opraveny duplicitn� zpr�vy a ozn�men�
 * Limit pro oznamov�n� zpr�v skupinov�ch rozhovor� - max. jednou za 15 sekund
 * Optimalizace pro odes�l�n� informace o psan�
 * Optimalizace pro zav�r�n� oken se zpr�vami na webu
 + Upozorn�n�, pokud je mo�n�, �e jsme nep�ijali n�kterou zpr�vu
 + P�i odesl�n� offline zpr�vy zobrazen� ozn�men� s mo�nost� otev��t odesl�n� zpr�vy skrz web

0.0.2.3 - 5.3.2011
 ! Oprava nena��t�n� jmen kontakt�
 ! Oprava definic (x64 verze) pro Updater
 + P�id�ny velk� (32px) stavov� ikony
 + Volba typu p��sp�vk�, kter� se budou oznamovat
 * Zm�na n�zv� User-Agent� na lid�t�j��
 * Vylep�eno parsov�n� novinek ze zdi
 
0.0.2.2 - 2.3.2011
 + Podpora pro Updater
 + Polo�ka v menu kontaktu a stavu pro otev�en� profilov� str�nky (+ ukl�d�n� v db jako polo�ka Homepage)
 + Oznamov�n� o nov� p�ijat�ch soukrom�ch zpr�v�ch (pouze ve stavu Neviditeln�)
 + Automatick� nastaven� https p�ipojen� p�i p�ihl�en�, pokud je vy�adov�no
 * Optimalizace stahov�n� avatar� kontakt�
 ! Oprava pro ob�asn� \n v ozn�men�ch+ 5 pokus� o odesl�n� zpr�vy p�ed zobrazen�m chybov� hl�ky.
 ! Oprava pro html tagy v chybov� hl�ce p�i p�ipojen�
 ! Oprava nemo�nosti odes�lat zpr�vy

0.0.2.1 - 21.2.2011
 ! Opravy dal��ch stav� (nemo�nost p�epnut� do Na chv�li pry�, p�ep�n�n� v jin�ch vl�knech,...)
 ! Oprava na�ten� vlastn�ho avataru p�i zm�n�
 * Nastaven� zvuk� pou��v� konkr�tn� n�zev ��tu (d�ky FREAK_THEMIGHTY)
 ! Opravy pro x64 verzi (d�ky FREAK_THEMIGHTY)
 ! Oprava synchronizace vl�ken 
 ! Opraveno po�ad� odes�lan�ch zpr�v a oznamov�n� o jej�ch chyb�ch
 ! Oprava p�r chyb zp�sobuj�c�ch nedoru�ov�n� v�ech zpr�v
 ! Opravy intern� implementace seznamu
 * Refactoring a zjednodu�en� n�kter�ch v�c�
 + P�id�na 64 bitov� verze pluginu
 + P�i p�ihl�en� ozn�men� o po�tu nov�ch notifikac�
 ! Oprava parsov�n� n�kter�ch typ� novinek
 + 1. f�ze podpory skupinov�ch chat� - oznamov�n� p��choz�ch zpr�v

0.0.2.0 - 13.2.2011
 * Vedeno jako nov� plugin Facebook RM + nov� readme a adres��ov� struktura
 x Do�asn� vypnuta podpora pro updater a informov�n� o "fb api"
 ! Oprava na�ten� vlastn�ho avataru
 ! Oprava zobrazov�n� novinek na zdi a jejich lep�� parsov�n�
 + P�id�ny stavy Na chv�li pry� a Neviditeln�
 * Stav Na chv�li pry� se pou��v� stejn� jako idle p��znak
 ! Opravena spr�va ne�innosti - facebook pad� do ne�innosti jen ve stavu Na chv�li pry�

--------------------------------
      Star� historie verz�
 (integrov�no v ofici�ln� verzi)
--------------------------------
0.0.2.0 (0.1.3.0) - 20.12.2010 (nevyd�no)
 ! Oprava zobrazov�n� bublinov�ho ozn�men�
 ! Oprava odhla�ovac� procedury (nekompletn�)
 ! Oprava zobrazen� ozn�men� s po�tem nov�ch zpr�v
 x Zru�eno na��t�n� stavov�ch zpr�v (facebook je p�estal zobrazovat)
 * Zm�na na��t�n� avatar� z hovercardu m�sto mobiln�ho facebooku (zrychlen� stahov�n� a sn�en� velikosti dat)
 + Kontrola �sp�n�ho odesl�n� zpr�vy
 * Aktualizov�na modifikovan� miranda32.exe na nejnov�j�� verzi
 * Rozd�leno readme na �esk� a anglick� a aktualizov�no
1.21 - 27.11.2010
 + Oznamov�n� nov� p�ijat�ch "soukrom�ch" (ne chatov�ch) zpr�v
 + Pseudo spr�va ne�innosti (p�i ne�innosti v mirand� nech� facebook upadnout do jeho ne�innosti)
 + Skryt� polo�ka v datab�zi, pro ignorov�n� timeout� kan�lu zpr�v (vlo�it kl�� "DisableChannelTimeouts" (byte) s hodnotou 1)
 * P�eps�ny a upraveny n�kter� v�ci (mo�n� opraveny duplicitn� zpr�vy, mo�n� p�id�ny dal�� chyby, apod.)
 ! Opravena zm�na ne�innosti kontakt�
 ! Oprava v�pisu �asu v logu
1.20 - 22.11.2010
 + Mo�nost pou�it� bublinov�ho ozn�men� v li�t� m�sto popup�
 + Mo�nost pou�it� https protokolu (pom��e p�ipojen� p�i n�kter�ch firewallech)
 + Upraven� miranda32.exe je p�id�n pro spr�vnou funk�nost bez odpojov�n� fb (dokud n�kdo nep�ep�e http komunikaci :)
1.19 - 20.11.2010
 ! Oprava stahov�n� a aktualizace avatar�
 ! Nenastavov�n� "Vlastn�ho jm�na" kontakt� (tzn. zobrazen� nov� p�ezd�vky p�i zm�n�)
 ! Oprava html tag� ve stavov�ch zpr�v�ch
 ! Zobrazen� chyby p�i pokusu o odesl�n� zpr�vy v offline/offline kontaktu
 ! Pokus o opravu p�d� p�i moc dlouh�m textu p��sp�vku ze zdi
 ! Do�asn� ignorov�n� timeout chyb (dokud neoprav� v j�d�e mirandy)
1.18 - 28.9.2010
 ! Oprava p�i vynucen�m p�epojen�.
 ! Nerozbrazov�n� odpojen� kontakt� p�i vlastn�m odpojen�
1.17 - 23.9.2010
 ! Pokus o vy�e�en� duplicitn�ch zpr�v
1.16 - 10.9.2010
 * Po p�ihl�en� se nezobrazuj� star� novinky ze zdi (pro star� chov�n� vytvo�it BYTE kl�� OldFeedsTimestamp s hodnotou 1)
 ! Oprava pro pr�zdn� novinky na zdi (pr�zdn� se nezobraz�)
1.15 - 9.9.2010
 ! Oprava pro nastaven� -1 (nekone�n�ho) �asu ozn�men�
1.14 - 9.9.2010
 ! Opravena v ozn�men�ch /span>
1.13 - 9.9.2010
 ! Zav�r�n� fb okna se zpr�vou (p�i p�ij�m�n� zpr�vy) je zpracov�no v jin�m threadu.
1.12 - 8.9.2010
 ! Oprava chyby p�i odpojen� posledn�ho kontaktu.
1.11 - 7.9.2010
 ! Oprava pro duplicitn� ozn�men� novinek, kter� nemaj� odkaz (zm�na fotky, atd.)
1.10 - 7.9.2010
 + Z�kladn� otv�r�n� facebook str�nky po kliknut� lev�ho tla��tka my�i
 + P�id�no nastaven� ozn�men� (barvy, �asy,...)
 + Informov�n� o KONKR�TN� chyb� p�i ne�sp�n�m pokusu o p�ihl�en�.
1.9 - 4.9.2010
 + Informov�n� o psan� kontaktu
1.8 - 4.9.2010
 + Mo�nost automaticky zav�rat okna se zpr�vou (na webu)
1.7 (0.1.2.0) - 7.8.2010
 ! Nezobrazov�n� Mirand�ho dialogu pro zad�n� stavov� zpr�vy, pokud nen� aktivov�na volba "Set Facebook 'Whats on my mind' status through Miranda status.
 ! Opraveno n�kolik �nik� pam�ti, ale n�kter� tu je�t� m��ou b�t...
 + P�id�n �as do debug logu.
1.6 (0.1.1.0) - 15.6.2010
 ! Oprava znovu nefunk�n�ho p�ipojen� p�i "Ozn�men� p�i p�ipojen� z jin�ho za��zen�".
 x Nefunguje nastavov�n�/maz�n� vlastn� "Co se v�m hon� hlavou?" zpr�vy
1.5 - 10.6.2010
 ! Oprava ozn�men� o nov�m Facebook API
 ! Opraven bug s k�dov�n�m zpr�v p�i pou�it� metakontakt�
 ! Oprava jedn� mal� chyby
1.4 - 7.6.2010 14:45
 ! Oprava z�sk�n� stavov� zpr�vy (GetMyAwayMsg) pro ansi pluginy (nap�. mydetails)
 + P�id�n zvuk a z�kladn� ikona FB k ozn�men�
1.3 - 7.6.2010 01:00
 ! Oprava nekon��c� "aktualizace informac� kontaktu"
 ! Oprava kdy se p�i odpojen� pluginu nastavila offline glob�ln� ikona
 ! Oprava na��t�n� feed� z facebook zdi
1.2
 ! Funguje p�ihla�ov�n� i se zapnut�m "Ozn�nem�m p�i p�ipojen� z jin�ho za��zen�.
1.1
 ! Oprava nefunk�n�ho p�ihl�en�
1.0
 + P�id�n skryt� kl�� v datab�zi: "Slo�ka" Facebook, kl�� DisableStatusNotify (typ Byte) s hodnotou 1. (Nov� p�idan� kontakty budou m�t automaticky nastaven p��znak "Ignorovat ozn�men� o zm�n� stavu" (tzn. to co se nastauvje v Mo�nosti / Ud�losti / Filtrov�n�)